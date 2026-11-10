# Nitpick documentation

Start with [INSTALL.md](https://github.com/alternative-intelligence-cp/nitpick/blob/main/INSTALL.md) for the source installation, LLVM tools, PATH setup and first program.
Save the `npkbuild` helper from its first-program section.
For `hello.npk`, use:

```sh
npkbuild hello
./hello
```

Read these guides in order:

1. [Quick start](01_QUICK_START.md): a first program, bindings, functions and `Result`, errors and `failsafe`, control flow, structs, and a library import.
2. [Cheat sheet](02_CHEAT_SHEET.md): short examples of calls, operators, types and literals.
3. [Driver guide](03_DRIVER_MODEL_GUIDE.md): the SDK header, C build setup, library paths, and a Nitpick client with a C driver.

The guides and [examples](https://github.com/alternative-intelligence-cp/nitpick-docs/tree/main/examples) come from the public [documentation repository](https://github.com/alternative-intelligence-cp/nitpick-docs), on branch `main`, with the guides at its top level.
Clone it beside the compiler clone, so that `nitpick/` and `nitpick-docs/` share a parent folder.
For the hello example, work in `examples/01_hello/`: run `npkbuild hello`, then `./hello`.
See the driver guide for `examples/02_driver/` and its build commands.
For further reading in the clone, see the reference files in `meta/specs/`.

Checked against compiler commit 64861f8.
