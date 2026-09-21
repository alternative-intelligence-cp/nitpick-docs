# Nitpick Quick Start Guide

Welcome to **Nitpick**, a modern systems programming language engineered for formal safety, mechanical determinism, zero undefined behavior, and out-of-process legacy interop.

This guide will walk you through the toolchain prerequisites, project structure, writing your first canonical "Hello World" program, and compiling it to a standalone, native static executable.

---

## 1. Toolchain Prerequisites

Nitpick compiles directly to LLVM Intermediate Representation (IR), which is assembled and linked by standard LLVM utilities:

| Tool | Purpose | Ubuntu / Debian Package |
|---|---|---|
| `npkc` | Nitpick native compiler frontend (emits `.ll`) | Built from Nitpick repository |
| `npkrt.o` | Nitpick runtime object (entrypoint, slab allocator, IPC) | Built from Nitpick repository |
| `llc` | LLVM static compiler (lowers `.ll` to `.o`) | `llvm` |
| `ld.lld` | High-performance LLVM linker (links static binary) | `lld` |
| `python3` | Test harness and benchmark automation | `python3` |

### Verifying Tools on Your System

Ensure LLVM tools are in your `PATH`:
```bash
llc --version
ld.lld --version
```

The Nitpick compiler binary and runtime live in the Nitpick repository's `build/` directory:
- Compiler frontend: `REPOS/nitpick/build/npkc`
- Runtime object: `REPOS/nitpick/build/npkrt.o`

---

## 2. Your First Program: Canonical "Hello World"

Create a new file named `hello.npk`:

```nitpick
mod:hello;
error:Err;

use "REPOS/nitpick/lib/nio.npk".*;

// Canonical Nitpick Entry Points
pub func:main = int32(cstring[]:_~argv) {
    // Acquire stdout handle
    NioW:out = raw nio_stdout();
    
    // Write line with emphatic unwrap (?! Err escalates failure to failsafe)
    nio_line(@out, "Hello from Nitpick!") ?! Err;
    nio_end(@out) ?! Err;
    
    // Explicit return code from main
    exit 0i32;
};

// Global deterministic failsafe handler
pub func:failsafe = int32(Error:e) {
    pick (e) {
        (Err)            { exit 1i32; },
        (IntOverflow)    { exit 93i32; },
        (OutOfBounds)    { exit 94i32; },
        (HeapBadRequest) { exit 91i32; },
        (HeapOom)        { exit 92i32; },
        (Unreachable)    { exit 95i32; },
        (WildLeak)       { exit 96i32; },
        (StackExhausted) { exit 97i32; },
        (MachineFault)   { exit 98i32; },
        (*)              { exit 99i32; }
    }
    exit 99i32;
};
```

---

## 3. How to Compile and Run

There are two primary ways to build Nitpick applications: **Method A** using the automated test harness, or **Method B** invoking the direct compiler pipeline.

### Method A: Using the Automated Runner (Fastest)

Nitpick provides a lightweight Python runner harness (`runner.py`) that handles LLVM IR generation, assembly, and static linking in one step:

```bash
# Debug build (no optimizations, fast turnaround)
python3 META/NITPICK/tests/tools/runner.py hello.npk

# Optimized build with LLVM -O2
python3 META/NITPICK/tests/tools/runner.py --opt hello.npk
```

**Output:**
```
[*] Compiling hello.npk (opt=False)...
[+] Built tests/.build/hello
[*] Executing tests/.build/hello...
Hello from Nitpick!
[*] Finished in 0.66 ms with exit code 0
```

---

### Method B: Direct CLI Pipeline (Step-by-Step)

If you want to understand how the pipeline works under the hood or integrate with existing Makefiles/build systems, run the three core stages directly:

#### Step 1: Compile Nitpick Source to LLVM IR
```bash
/path/to/nitpick/build/npkc hello.npk -o hello.ll
```
This performs syntax parsing, type checking, borrow and alias analysis, formal contract verification, and lowers the AST into native LLVM IR (`hello.ll`).

#### Step 2: Assemble LLVM IR to Machine Object Code
```bash
llc -O0 -filetype=obj -relocation-model=static hello.ll -o hello.o
```
*(For release builds, replace `-O0` with `-O2`).*

#### Step 3: Statically Link with the Nitpick Runtime
```bash
ld.lld -static -o hello hello.o /path/to/nitpick/build/npkrt.o
```
This produces a completely standalone, dependency-free static binary `hello`.

#### Step 4: Execute
```bash
./hello
# Hello from Nitpick!

echo $?
# 0
```

---

## 4. Fundamental Rules Every Nitpick Developer Must Know

If you are coming from C, Rust, Go, or Python, here are the core rules that define how Nitpick operates:

### 1. `exit()` is Restricted to `pub func:main` and `pub func:failsafe`
Normal functions (`func:` or `pub func:`) **cannot** call `exit()`. Doing so is undefined behavior. 
- Normal functions signal results using `pass(val)` or `fail(code)`.
- Only `pub func:main` and `pub func:failsafe` terminate the process with `exit(code)`.

### 2. Every Normal Function Returns a `Result<T>`
There is no unhandled `void` in Nitpick. Side effects can fail.
- `pass val;` returns a successful `Result<T>` containing `val`.
- `fail code;` returns a failure `Result<T>` carrying the error code.
- If a function has no payload value, it returns `Result<NIL>`:
  ```nitpick
  func:log_message = NIL(string:msg) {
      // ...
      pass NIL;
  };
  ```

### 3. Infallible Functions Must Explicitly Declare `never fails`
If a function can never produce an error (e.g., pure mathematical calculation), declare it with `never fails`. It returns the bare value directly, and callers can unwrap it at zero cost using `raw` or `_!`:
```nitpick
func:add_pure = int32(int32:a, int32:b) never fails {
    pass a +% b;
};

// Caller unwraps without error check:
int32:sum = raw add_pure(10i32, 20i32);
```

### 4. Unused Arguments Require Discard Sigil (`_~`)
To prevent accidental unused variables, arguments that are deliberately not read in the function body must be prefixed with `_~`:
```nitpick
pub func:main = int32(cstring[]:_~argv) {
    // argv is safely marked as unused
    exit 0i32;
};
```

### 5. Always Handle or Escalate Results
Nitpick does not permit silently dropping a `Result`. You must:
- Escalate to failsafe: `result ?! Err`
- Provide a fallback value: `val = result ?| default_val`
- Propagate to caller: `val = _^ result` (or `relay result`)
- Explicitly discard (if intentional): `drop result`
