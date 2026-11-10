# Nitpick quick start

## Install first

Follow [INSTALL.md](https://github.com/alternative-intelligence-cp/nitpick/blob/main/INSTALL.md) through its source build and installation steps, including the LLVM 20 tools and PATH setup.
Use its installed `npkc`, runtime object and `npkbuild` helper for this guide.

## A first program

Save this as `hello.npk` in a folder of your choice, as in INSTALL.md:

```nitpick
mod:hello;

func:main = int32(cstring[]:_~argv) {
    string:greeting = "hello from Nitpick\n";
    discard(write(1i32 =>! fd, greeting.ptr =>! wild int8->, greeting.len) ?! E9);
    exit 0i32;
};

error:E9;

func:failsafe = int32(Error:e) {
    pick (e) {
        (E9)             { exit 9i32; },
        (StackExhausted) { exit 97i32; },
        (MachineFault)   { exit 98i32; },
        (Unreachable)    { exit 95i32; },
        (HeapOom)        { exit 92i32; },
        (HeapBadRequest) { exit 91i32; },
        (WildLeak)       { exit 96i32; },
        (*)              { exit 99i32; }
    }
    exit 99i32;
};
```

Build and run it from that folder with the same commands as INSTALL.md:

```sh
npkc hello.npk -o hello.ll
llc -O0 -filetype=obj -relocation-model=static hello.ll -o hello.o
ld.lld -static -o hello hello.o ~/.local/lib/nitpick/npkrt.o
./hello
```

What's in it, as in INSTALL.md section 5:

- `main` takes the command line and returns an `int32`. `exit` stops the program and is legal only in `main` and `failsafe`.
- `write` makes one kernel write to file descriptor 1. Its `Result` is followed by `?! E9` to take the value or send `E9` to `failsafe` on failure. `discard` explicitly discards the returned byte count.
- `failsafe` receives uncaught errors and must name every error that can reach it, as well as include `(*)`.

It prints `hello from Nitpick` and exits with status 0.
The `mod:hello;` declaration matches the file name `hello.npk`.
The compiler writes LLVM text to `hello.ll`; the next two commands assemble and link it.
After saving INSTALL.md's `npkbuild` helper, the shorter form is:

```sh
npkbuild hello
./hello
```

## Bindings and types

For each section from here through Structs, replace the entire body of `main` in `hello.npk` with that section's main-body fragment followed by `exit 0i32;`.
Keep the `mod:hello;` line, earlier module-scope declarations and `failsafe` arms. Add declarations at module scope where directed. After each section, save `hello.npk`, build it with `npkbuild hello`, and run `./hello`; these sections exit with status 0 and print nothing.
The compiler lists every error that can reach `failsafe` and asks for missing arms by name.
Before this fragment, add `(IntOverflow) { exit 93i32; },` before the `(E9)` arm in `failsafe`:

```nitpick
int32:count = 2i32;
count = count + 1i32;
fixed int32:ceiling = count + 1i32;
string:label = "count";
if (count != 3i32 || ceiling != 4i32 || label.len != 5i64) { exit 1i32; }
```

`int32:count` puts the type before the name; `3i32` supplies an integer type suffix.
Here `count` is reassigned. `fixed` makes `ceiling` write-once: it can be initialized at runtime, but reassignment is refused with `NITPICK-ASSIGN-002`.

## Functions and Result

Add this fragment at module scope, outside `main`:

```nitpick
func:answer = int32(int32:unused) never fails {
    pass 42i32;
};

func:note = NIL() never fails { pass NIL; };
```

Use this fragment inside `main`, before its final `exit`:

```nitpick
Result<int32>:r = answer(0i32);
if (r.is_error) { exit 1i32; }
if (r.value != 42i32) { exit 2i32; }
int32:n = raw answer(0i32);
if (n != 42i32) { exit 3i32; }
drop note();
```

Even a `never fails` call returns `Result<int32>`; `raw` unwraps it to `int32`.
The unused parameter `unused` compiles without `_~`. Prefixing a parameter with `_~`, as in `_~argv`, forbids reading it in the function body.
`drop` discards the `NIL` result of the `never fails` call `note()`.
Using `drop` on a fallible call is refused with `NITPICK-TYPE-042`.
An ordinary function containing `exit` is refused with `NITPICK-TYPE-010`; use `pass` for success or `fail` for an error.

## Errors and failsafe

Before adding this section's fragments, add `(MissingItem) { exit 10i32; },` before the `(E9)` arm in `failsafe`.

Add these declarations as a fragment at module scope:

```nitpick
error:MissingItem;

func:lookup = int32(bool:present) {
    if (!present) { fail MissingItem; }
    pass 7i32;
};

func:forward = int32(bool:present) {
    pass (relay lookup(present));
};
```

Then try this fragment inside `main`, before its final `exit`:

```nitpick
int32:fallback = lookup(false) ?| 12i32;
int32:found = forward(true) ?! E9;
if (fallback != 12i32 || found != 7i32) { exit 1i32; }
```

`fail MissingItem` returns an error, `?|` supplies a fallback, and `relay` forwards a failure to the caller.
`?! E9` takes the successful value or sends `E9` to `failsafe` on failure.
A wildcard `(*)` alone does not replace named reachable-error arms: the compiler reports `NITPICK-REACH-002`.
`MissingItem` is a local error name; redeclaring the prelude's `NotFound` is refused with `NITPICK-RESOLVE-001`.

## Control flow and pick

Try this fragment inside `main`, before its final `exit`:

```nitpick
int32:sum = 0i32;
for (int32:i in 1i32..3i32) { sum = sum + i; }
int32:count = 0i32;
for (int32:j in 0i32...3i32) { count = count + 1i32; }
if (count != 3i32) { exit 1i32; }
pick (sum) {
    (6i32) { count = count + 1i32; },
    (*) { exit 2i32; }
}
if (count != 4i32) { exit 3i32; }
```

`1i32..3i32` includes 3; `0i32...3i32` stops before 3.
In this example, the `6i32` arm runs and the wildcard arm does not run.

## Structs

Add this fragment at module scope:

```nitpick
struct:Point = { int32:x; int32:y; };
```

Use this fragment inside `main`, before its final `exit`:

```nitpick
Point:p = Point{ x: 3i32, y: 4i32 };
p.y = 5i32;
if (p.x != 3i32 || p.y != 5i32) { exit 1i32; }
```

`Point{ ... }` initializes the fields, and `p.y` reads or writes a field.

## A library through use

Save this separate program as `library.npk` at the root of the compiler clone, beside `lib/`:

```nitpick
mod:library;

use "./lib/nio.npk".*;
error:E9;

func:main = int32(cstring[]:_~argv) {
    NioW:out = raw nio_stdout();
    nio_line(@out, "hello from a library") ?! E9;
    nio_end(@out) ?! E9;
    exit 0i32;
};

func:failsafe = int32(Error:e) {
    pick (e) {
        (E9) { exit 80i32; },
        (DecreasesViolated) { exit 81i32; },
        (IntOverflow) { exit 82i32; },
        (OutOfBounds) { exit 83i32; },
        (Unreachable) { exit 84i32; },
        (HeapOom) { exit 85i32; },
        (HeapBadRequest) { exit 86i32; },
        (WildLeak) { exit 87i32; },
        (StackExhausted) { exit 88i32; },
        (MachineFault) { exit 89i32; },
        (*) { exit 99i32; }
    }
    exit 99i32;
};
```

Build it with `npkbuild library` from the clone root, then run `./library`.
It prints `hello from a library` and exits with status 0.
The `use` path is relative to this source file; `@out` supplies the writer's address.

For further reading, see the reference files in `meta/specs/` in the compiler clone.

Checked against compiler commit 64861f8.
