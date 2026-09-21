# Nitpick Documentation

Welcome to the official developer documentation repository for the **Nitpick** programming language.

Nitpick is a modern systems programming language built from the ground up for formal correctness, deterministic memory safety without a garbage collector, zero undefined behavior, and high-performance out-of-process legacy interoperability.

---

## Documentation Guides

### 1. [Quick Start Guide](01_QUICK_START.md)
* Prerequisites, LLVM toolchain dependencies (`llc`, `ld.lld`).
* Writing your first canonical "Hello World" application.
* Compiling via the automated runner harness vs. direct CLI commands.
* Core language rules for beginners (`pub func:main`, `pub func:failsafe`, `Result<T>` propagation).

### 2. [Language Cheat Sheet](02_CHEAT_SHEET.md)
* Complete 19-level operator precedence table.
* First-class wrapping arithmetic (`+%`, `-%`, `*%`), error escalation (`?!`, `_~`, `_^`), and memory claims (`$$i`, `$$m`).
* Bit-precise scalar types (`int8`..`int4096`), fixed-point/rationals (`frac`, `tbb`, `tfp`), and runtime containers (`Handle`, `arena`, `atomic`, `Result`, `Bridge`).
* Declarations, modifiers, verification clauses (`never fails`, `requires`, `ensures`), and compiler intrinsics (`NitpickAlloc`).

### 3. [Out-of-Process Driver Model & Bridge Guide](03_DRIVER_MODEL_GUIDE.md)
* Architectural motivation: why in-process FFI is unsafe and how supervised isolation prevents memory corruption.
* Wire Protocol v3: control socket (fd 3) and sealed shared memory ring buffer (`NPKDRV03`).
* Writing C drivers using the `sdk/npkdrv.h` header.
* Consuming drivers in Nitpick via `extern:"driver"` and asynchronous `await`.
* Fault containment: intercepting C segfaults as `EDriverFault` while keeping the host runtime alive.

---

## Editor Support

Official syntax highlighting, bracket matching, and code snippets for **Antigravity IDE** and **VSCode** are available in the [vscode-nitpick](file:///home/randy/Workspace/META/NITPICK-LIBS/vscode-nitpick) extension.
