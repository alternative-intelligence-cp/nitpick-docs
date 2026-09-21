# Nitpick Out-of-Process Driver Model & Bridge Guide

This guide explains Nitpick's out-of-process driver architecture (Protocol Wire v3, D-149), which allows Nitpick applications to interact with legacy C/C++, graphics libraries (OpenGL, Vulkan, GTK4), hardware APIs, and CUDA with zero performance compromises and complete fault isolation.

---

## 1. Why Out-of-Process Isolation?

In traditional systems languages (C++, Rust, Zig, Go), Foreign Function Interface (FFI) runs **in-process**. If a C library has a buffer overflow, dereferences a null pointer, or corrupts heap metadata, it detonates the entire host process. All safety guarantees are destroyed the instant an unverified C library is linked.

Nitpick solves this through **Supervised Out-of-Process Drivers**:

```
+-----------------------------------------------------------------------+
|                         NITPICK RUNTIME (TCB)                         |
|                                                                       |
|  pub func:main                                                        |
|    await render_ppm(@b, 400i32, 300i32, ...)                          |
+-----------------------------------+-----------------------------------+
                                    |
          Control Socket (fd 3)     |  Sealed Shared Memory (memfd)
          INIT_REQ / EXEC_NOTIFY    |  32-byte Ring Descriptors + Slices
                                    |
+-----------------------------------+-----------------------------------+
|                        SUPERVISED C DRIVER                            |
|                     (Outside the Nitpick TCB)                         |
|                                                                       |
|  canvas_driver: handles OpenGL / GTK / CUDA / C libraries             |
|  If it segfaults -> Nitpick catches EDriverFault; host stays alive!   |
+-----------------------------------------------------------------------+
```

### Core Architectural Guarantees
1. **Zero-Copy Performance**: Communication happens over a sealed shared memory region (`memfd`). Bulk image, audio, or tensor slices are passed without copying data between user and kernel space.
2. **Fault Containment (The Firewall)**: The driver runs in a separate process monitored via Linux `pidfd`. If the C driver segfaults, divides by zero, or hits an assertion, the Nitpick process intercepts it as `EDriverFault`. The host runtime's heap and execution remain completely untouched.
3. **No Orphan Processes**: When Nitpick terminates (cleanly or via failsafe trap), the supervisor registry kills all associated drivers immediately.

---

## 2. The Wire Protocol (v3)

Communication consists of two planes:

### A. The Control Plane (UNIX Domain Socket, fd 3)
* **`0x0001 INIT_REQ`** (Bridge -> Driver): Passes shared memory capacity and interface hash, with the sealed `memfd` transferred via `SCM_RIGHTS`.
* **`0x8001 INIT_ACK`** (Driver -> Bridge): Acknowledges initialization.
* **`0x0002 EXEC_NOTIFY`** (Bridge -> Driver): Signals that a new job descriptor is available on the ring.
* **`0x8002 WORK_COMPLETE`** (Driver -> Bridge): Signals job completion with status code (0 = success).
* **`0x00FF SHUTDOWN`** (Bridge -> Driver): Advisory teardown notification.

### B. The Data Plane (Sealed Shared Memory Ring)
* **Header Magic (`0x00`)**: `0x4E504B4452563033` (`"NPKDRV03"`).
* **Ring Counters**: C11 atomic head (`0x40`) and tail (`0x80`) offsets.
* **Descriptor Ring (`0x100`)**: Ring of 32-byte records:
  ```c
  struct {
      uint32_t seq;
      uint32_t kernel_id;
      uint64_t arg_off;
      uint64_t arg_len;
      uint64_t resp_off;
  };
  ```
* **Bulk Buffer**: Dedicated shared memory area for zero-copy slice passing.

### C. The Interface Hash
To prevent type confusion or mismatched ABIs between a Nitpick application and a driver binary, both sides compute an **Interface Hash** over the canonical method spellings using FNV-1a basis `0xCBF5DAE484222325` and prime `0x100000001B3`. If the signatures do not match identically, the driver refuses initialization immediately.

---

## 3. Implementing a Driver in C (Using `sdk/npkdrv.h`)

The Nitpick repository provides the single-header C driver SDK in `sdk/npkdrv.h`.

Here is a complete, minimal C driver (`canvas_driver.c`):

```c
#include "REPOS/nitpick/sdk/npkdrv.h"

// 1. Declare canonical interface signatures in exact declaration order (kernel_id 0..N)
static const char *const CANON[] = {
    "ping=int64(int64)",
    "render_ppm=int64(int32,int32,int32)",
    "filter_image=int64(uint8[],int32,int32)",
    "crash_driver=NIL()",
    NULL,
};

int main(void) {
    npkdrv d;
    uint64_t hash = npkdrv_iface_hash(CANON);
    
    // 2. Initialize driver and handshake with Nitpick over control socket fd 3
    if (npkdrv_init(&d, hash) != 0) {
        return 10;
    }

    // 3. Dispatch loop
    for (;;) {
        npkdrv_desc req;
        int r = npkdrv_next(&d, &req);
        if (r == 0) return 0; // Clean shutdown requested by Nitpick
        if (r < 0) return 11;

        switch (req.kernel_id) {
        case 0: { // ping(int64 x) -> int64
            int64_t x;
            memcpy(&x, d.shm + req.arg_off, 8);
            int64_t ret = x + 1;
            memcpy(d.shm + req.resp_off, &ret, 8);
            if (npkdrv_complete(req.seq, 0) != 0) return 12;
            break;
        }

        case 1: { // render_ppm(int32 width, int32 height, int32 color) -> int64
            int64_t w_slot, h_slot, c_slot;
            memcpy(&w_slot, d.shm + req.arg_off + 0, 8);
            memcpy(&h_slot, d.shm + req.arg_off + 8, 8);
            memcpy(&c_slot, d.shm + req.arg_off + 16, 8);
            
            // Execute rendering logic...
            int64_t pixels = w_slot * h_slot;
            memcpy(d.shm + req.resp_off, &pixels, 8);
            if (npkdrv_complete(req.seq, 0) != 0) return 12;
            break;
        }

        case 2: { // filter_image(uint8[] pixels, int32 width, int32 height) -> int64
            // Slice layout: slot 0 = relative offset from bulk base, slot 1 = length
            uint64_t rel_off, len;
            memcpy(&rel_off, d.shm + req.arg_off + 0, 8);
            memcpy(&len, d.shm + req.arg_off + 8, 8);
            
            uint8_t *pixel_data = d.bulk + rel_off;
            int64_t sum = 0;
            for (uint64_t i = 0; i < len; i++) {
                sum += pixel_data[i];
            }
            memcpy(d.shm + req.resp_off, &sum, 8);
            if (npkdrv_complete(req.seq, 0) != 0) return 12;
            break;
        }

        case 3: { // crash_driver() -> NIL
            // Intentionally dereference NULL to demonstrate fault containment
            volatile int *null_ptr = NULL;
            *null_ptr = 42; 
            break;
        }
        }
    }
}
```

### Compiling the C Driver
```bash
clang -O2 -Wall -Wextra canvas_driver.c -o canvas_driver
```

---

## 4. Consuming the Driver in Nitpick

On the Nitpick side, declare the driver interface using an `extern:"driver_name"` block:

```nitpick
mod:gui_app;
error:Err;

use "REPOS/nitpick/lib/nbridge.npk".*;
use "REPOS/nitpick/lib/nio.npk".*;

// 1. Declare external driver interface. Methods map 0-indexed to kernel_id
extern:"canvas_drv" = {
    func:ping         = int64(Bridge->:b, int64:val, Duration:within);
    func:render_ppm   = int64(Bridge->:b, int32:width, int32:height, int32:color, Duration:within);
    func:filter_image = int64(Bridge->:b, uint8[]:pixels, int32:width, int32:height, Duration:within);
    func:crash_driver = NIL(Bridge->:b, Duration:within);
};

pub async func:main = int32(cstring[]:argv) {
    if (argv.len < 2i64) { exit 1i32; }

    Path:p = path_parse(string_from_bytes(argv[1i64].ptr, argv[1i64].len)) ?! Err;
    int64:hash = raw canvas_drv_iface_hash();
    NioW:out = raw nio_stdout();

    // 2. Spawn driver over shared memory ring (64KB shm, 64-slot ring, 5s timeout)
    Bridge:b = await spawn_driver(p, 65536i64, 64i64, hash, raw duration_secs(5i64)) ?! Err;

    // 3. Make asynchronous calls
    Result<int64>:pr = await ping(@b, 42i64, raw duration_secs(5i64));
    if (pr.is_error) { exit 2i32; }

    // 4. Pass zero-copy slice over ring
    uint8[4]:buf = [10u8, 20u8, 30u8, 40u8];
    uint8[]:slice = buf[0i64...4i64];
    Result<int64>:fr = await filter_image(@b, slice, 2i32, 2i32, raw duration_secs(5i64));
    
    // 5. Fault Containment Test: Driver crash does NOT crash Nitpick!
    nio_line(@out, "[*] Triggering C crash...") ?! Err;
    Result<NIL>:cr = await crash_driver(@b, raw duration_secs(5i64));
    
    if (cr.is_error && cr.err == EDriverFault) {
        nio_line(@out, "[+] Driver crashed with SIGSEGV, but caught safely as EDriverFault!") ?! Err;
        nio_line(@out, "[+] Nitpick host runtime is completely intact.") ?! Err;
    }

    // 6. Clean teardown
    drop bridge_reap(@b);
    exit 0i32;
};

pub func:failsafe = int32(Error:e) {
    pick (e) {
        (EDriverFault)    { exit 94i32; },
        (EDriverDeadline) { exit 96i32; },
        (*)               { exit 99i32; }
    }
    exit 99i32;
};
```

---

## 5. Performance Characteristics

In verified benchmarks against Linux IPC mechanisms:
- **Ping-Pong Latency**: Over 10,000 round-trip calls over the atomic shared memory ring execute in **sub-millisecond** average times, bypassing standard socket context-switching overhead.
- **Bulk Slices**: Transferring 4K image frames or audio buffers incurs zero memory copying: the driver maps the physical pages directly from the sealed memfd.
- **Teardown**: When Nitpick closes, `bridge_reap` safely reaps the child pidfd; if a crash occurs, the supervisor registry ensures no zombie driver processes linger.
