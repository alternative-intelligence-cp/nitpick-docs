# Driver model and Bridge guide

## Files and tools

Follow [INSTALL.md](https://github.com/alternative-intelligence-cp/nitpick/blob/main/INSTALL.md) for the source installation and keep the compiler clone.
Use the SDK header at `sdk/npkdrv.h` in that clone; no separate header download is needed.
This example also uses the clone's `lib/nbridge.npk` and `lib/nio.npk`.
The driver needs a C11 compiler available as `cc` and Linux C development headers, in addition to INSTALL.md's tools.
GCC or Clang can provide `cc`; the commands below use `cc`.

Clone the public [documentation repository](https://github.com/alternative-intelligence-cp/nitpick-docs) beside the compiler clone, so that `nitpick/` and `nitpick-docs/` share a parent folder.
Use `canvas_driver.c` and `gui_bridge_poc.npk` in `nitpick-docs/examples/02_driver/`.
Run every command below from that folder.

## The C driver

Save this as `canvas_driver.c`:

```c
#include "npkdrv.h"

static const char *const CANON[] = {
    "ping=int64(int64)",
    "render_ppm=int64(int32,int32,int32)",
    "filter_image=int64(uint8[],int32,int32)",
    NULL,
};

int main(void) {
    npkdrv d;
    if (npkdrv_init(&d, npkdrv_iface_hash(CANON)) != 0) return 10;
    for (;;) {
        npkdrv_desc req;
        int r = npkdrv_next(&d, &req);
        if (r == 0) return 0;
        if (r < 0) return 11;
        switch (req.kernel_id) {
        case 0: {
            int64_t x;
            memcpy(&x, d.shm + req.arg_off, 8);
            int64_t reply = x + 1;
            memcpy(d.shm + req.resp_off, &reply, 8);
            break;
        }
        case 1: {
            int64_t ws, hs, cs;
            memcpy(&ws, d.shm + req.arg_off, 8);
            memcpy(&hs, d.shm + req.arg_off + 8, 8);
            memcpy(&cs, d.shm + req.arg_off + 16, 8);
            int width = (int)ws, height = (int)hs;
            FILE *f = fopen("canvas_output.ppm", "wb");
            if (!f) return 13;
            fprintf(f, "P6\n%d %d\n255\n", width, height);
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    fputc(x * 255 / width, f);
                    fputc(y * 255 / height, f);
                    fputc((int)(cs & 255), f);
                }
            }
            if (fclose(f) != 0) return 14;
            int64_t pixels = (int64_t)width * height;
            memcpy(d.shm + req.resp_off, &pixels, 8);
            break;
        }
        case 2: {
            uint64_t rel_off, len;
            memcpy(&rel_off, d.shm + req.arg_off, 8);
            memcpy(&len, d.shm + req.arg_off + 8, 8);
            uint8_t *pixels = d.shm + d.bulk_offset + rel_off;
            int64_t sum = 0;
            for (uint64_t i = 0; i < len; i++) {
                pixels[i] = 255 - pixels[i];
                sum += pixels[i];
            }
            memcpy(d.shm + req.resp_off, &sum, 8);
            break;
        }
        default:
            return 15;
        }
        if (npkdrv_complete(req.seq, 0) != 0) return 12;
    }
}
```

Build it from the example folder with `cc -std=c11 -D_GNU_SOURCE -O2 -Wall -Wextra -I ../../../nitpick/sdk canvas_driver.c -o canvas_driver`.
The `-I ../../../nitpick/sdk` option supplies the include directory for `#include "npkdrv.h"`.
Keep the SDK header and library files from the same clone for this example.

## The Nitpick client

Save this as `gui_bridge_poc.npk` in `examples/02_driver/` (the supplied example has the same text):

```nitpick
mod:gui_bridge_poc;

use "../../../nitpick/lib/nbridge.npk".*;
use "../../../nitpick/lib/nio.npk".*;
error:E9;

extern:"canvas_drv" = {
    func:ping = int64(Bridge->:b, int64:val, Duration:within);
    func:render_ppm = int64(Bridge->:b, int32:width, int32:height, int32:color, Duration:within);
    func:filter_image = int64(Bridge->:b, uint8[]:pixels, int32:width, int32:height, Duration:within);
};

async func:main = int32(cstring[]:argv) {
    if (argv.len != 2i64) { exit 1i32; }
    string:driver_path = string_from_bytes(argv[1i64].ptr, argv[1i64].len);
    Path:p = path_parse(driver_path) ?! E9;
    int64:h = raw canvas_drv_iface_hash();
    Result<Bridge>:br = await spawn_driver(p, 65536i64, 64i64, h, raw duration_secs(5i64));
    if (br.is_error) { exit 1i32; }
    Bridge:b = move(br) ?! E9;

    Result<int64>:pr = await ping(@b, 42i64, raw duration_secs(5i64));
    if (pr.is_error) { exit 2i32; }
    if (pr.value != 43i64) { exit 3i32; }

    Result<int64>:rr = await render_ppm(@b, 400i32, 300i32, 255i32, raw duration_secs(5i64));
    if (rr.is_error) { exit 4i32; }
    if (rr.value != 120000i64) { exit 5i32; }

    uint8[4]:buf = [10u8, 20u8, 30u8, 40u8];
    uint8[]:slice = buf[0i64...4i64];
    Result<int64>:fr = await filter_image(@b, slice, 2i32, 2i32, raw duration_secs(5i64));
    if (fr.is_error) { exit 6i32; }
    if (fr.value != 920i64) { exit 7i32; }

    Result<NIL>:closed = await bridge_close(@b, raw duration_secs(3i64));
    if (closed.is_error) { exit 8i32; }
    drop bridge_reap(@b);
    NioW:out = raw nio_stdout();
    nio_line(@out, "driver calls completed") ?! E9;
    nio_end(@out) ?! E9;
    exit 0i32;
};

func:failsafe = int32(Error:e) {
    pick (e) {
        (E9) { exit 80i32; },
        (BadPath) { exit 81i32; },
        (EDriverSpawn) { exit 82i32; },
        (EDriverFault) { exit 83i32; },
        (EDriverProtocol) { exit 84i32; },
        (EDriverDeadline) { exit 85i32; },
        (EDriverError) { exit 86i32; },
        (ERingFull) { exit 87i32; },
        (EBridgePoisoned) { exit 88i32; },
        (EShmCreate) { exit 89i32; },
        (EShmSeal) { exit 90i32; },
        (EShmMap) { exit 91i32; },
        (DeadlineExceeded) { exit 92i32; },
        (DriverLeak) { exit 93i32; },
        (DecreasesViolated) { exit 94i32; },
        (IntOverflow) { exit 95i32; },
        (OutOfBounds) { exit 96i32; },
        (Unreachable) { exit 97i32; },
        (HeapOom) { exit 98i32; },
        (HeapBadRequest) { exit 99i32; },
        (WildLeak) { exit 100i32; },
        (StackExhausted) { exit 101i32; },
        (MachineFault) { exit 102i32; },
        (*) { exit 99i32; }
    }
    exit 99i32;
};
```

Each `use` path is relative to `gui_bridge_poc.npk`: `../../../nitpick/lib/nbridge.npk` reaches the compiler clone's `lib/nbridge.npk`.
The C signature table and the `extern` declarations list the same three methods in the same order.
In this example, `ping(42)` returns 43, `render_ppm(400, 300, 255)` returns 120000, and `filter_image` returns 920 for the four bytes shown.
The C driver writes `canvas_output.ppm` in the working directory, with a 400 by 300 image.
`move(br) ?! E9` takes the Bridge from the spawn result; `bridge_close` and `bridge_reap` finish this example's driver session.

## Build and run

Use the `npkbuild` helper from INSTALL.md after building the C driver:

```sh
npkbuild gui_bridge_poc
./gui_bridge_poc "$PWD/canvas_driver"
```

The client prints `driver calls completed` and exits with status 0.
Pass the driver as `"$PWD/canvas_driver"` from `examples/02_driver/`; the client reads that command-line argument with `path_parse`.
The SDK header contains the control-message constants and descriptor declarations for further reading.

## Low-level driver calls

The example above uses `spawn_driver` and `bridge_reap` from `lib/nbridge.npk`.
The lower-level `clone_exec(p)` takes a `wild any->` pointing to a ten-word parameter block: path, argv, envp, stdin/stdout/stderr sources, optional control descriptor, parent pid, and two output slots (registry slot and pidfd).
It starts a supervised child and returns its pid in a `Result<int64>`; see `meta/specs/BUILTIN_REFERENCE.md`, `clone_exec`, for the block layout and descriptor requirements.
`driver_retire(slot);` takes an `int64` registry slot and retires it after the child is dead and reaped; it returns `NIL`.
These module-scope fragments compile when added to a complete program, but this guide doesn't run them. They require the prepared block and live registry state described above; the complete library example is the runnable entry point.

```nitpick
func:start = int64(wild int8->:p) { pass (relay clone_exec(p)); };
```

```nitpick
func:retire = NIL(int64:slot) never fails { driver_retire(slot); pass NIL; };
```

Checked against compiler commit 64861f8.
