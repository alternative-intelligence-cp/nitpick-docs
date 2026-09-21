# Nitpick Language Cheat Sheet

A concise, authoritative reference of all operators, types, keywords, modifiers, contracts, and compiler built-ins in the Nitpick systems programming language.

---

## 1. Operator Reference & Precedence

Operators are listed from highest precedence (1) to lowest (19). Operator overloading is strictly forbidden.

| Level | Category | Operators | Description |
|---|---|---|---|
| 1 | Postfix | `()` `[]` `.` `?.` | Call, indexing, unified member access (auto-deref), safe navigation |
| **2** | **Result Unary** | `raw` `_!` · `drop` `_?` · `await` · `relay` `_^` | Checked unwrap, drop, async suspend, error propagation to caller |
| 3 | Pipeline | `\|>` `<\|` | Function pipeline forwarding |
| 4 | Cast | `=>` `=>!` | Checked compile-time cast, unchecked bitcast/truncation |
| 5 | Unary | `!` `~` `-` `@` `<-` `$$i` `$$m` | Logical NOT, bitwise NOT, negation, address-of, deref, shared/exclusive claims |
| 6 | Multiplicative | `*` `/` `%` `*%` | Multiplication, division, modulo, wrapping multiplication |
| 7 | Additive | `+` `-` `+%` `-%` | Addition, subtraction, wrapping addition, wrapping subtraction |
| 8 | Shift | `<<` `>>` | Left shift, right shift (amount must be $0 \le n < \text{width}$) |
| 9 | Range / Spread | `..` `...` `..*` `..^` | Half-open `[a, b)`, closed `[a, b]`, strided range, length range |
| 10 | Relational | `<` `<=` `>` `>=` `<=>` | Comparisons, spaceship (returns `-1`, `0`, or `1`) |
| 11 | Equality | `==` `!=` | Equality and inequality |
| 12 | Bitwise AND | `&` | Bitwise AND |
| 13 | Bitwise XOR | `^` | Bitwise XOR |
| 14 | Bitwise OR | `\|` | Bitwise OR |
| 15 | Logical AND | `&&` | Short-circuiting logical AND (strict boolean) |
| 16 | Logical OR | `\|\|` | Short-circuiting logical OR (strict boolean) |
| 17 | Null Coalesce | `??` | Unwraps `Optional<T>`. If `NIL`, yields RHS default |
| 18 | Result Fallback | `?\|` | Unwraps `Result<T>`. If error, yields RHS default |
| 19 | Assignment | `=` `+=` `-=` `*=` `/=` `%=` `+%=` `-%=` `*%=` `&=` `\|=` `^=` `<<=` `>>=` | In-place statements (assignment yields no value) |

---

## 2. Key Operator Families Explained

### Wrapping Arithmetic (D-312)
Standard `+`, `-`, `*` **trap** on overflow (`IntOverflow`). For algorithms where wrap-around is intentional (hashing, checksums, PRNGs, modular arithmetic), use the wrapping family:
* `+%`, `-%`, `*%`: Low-N bit operations modulo $2^N$. Zero trap overhead, zero verification obligations.
* `+%=`, `-%=`, `*%=`: Compound wrapping assignment (`h *%= 1099511628211u64;`).

### Result & Error Operators
* `?! Err`: **Emphatic Unwrap**. Unwraps `Result<T>`. If an error occurred, calls `failsafe(err)`.
* `?| val`: **Result Fallback**. Yields value if ok, else returns `val`.
* `_^ expr` (or `relay`): **Error Propagation**. If `expr` failed, returns immediately to caller with the same error; else yields `.value`.
* `_! expr` (or `raw`): **Zero-Cost Infallible Unwrap**. Unwraps a function proven with `never fails`.
* `_? expr` (or `drop`): Discards the `NIL` result of a `never fails` function.
* `_~ expr` / `Type:_~arg`: Discard statement or marks unused function parameter.
* `!!! err`: Immediately raises failsafe trap with `err`.

### Memory, Claims & Pointers
* `@val`: Takes memory address (`Type->`).
* `<-ptr`: Dereferences pointer to obtain value.
* `->`: Declares pointer type (`int32->:p`).
* `.`: Unified member access. Automatically dereferences pointers when accessing struct fields or UFCS methods.
* `$$i val`: Shared borrow claim (multiple readers, zero writers).
* `$$m val`: Exclusive borrow claim (single reader/writer, zero overlapping access).

---

## 3. Complete Type Inventory

### Scalar Integers (Tier 0)
Explicit bit-widths from 8 to 4096 bits. Unary `+ - *` trap on overflow; bit operations never trap.
* Signed: `int8`, `int16`, `int32`, `int64`, `int128`, `int256`, `int512`, `int1024`, `int2048`, `int4096`.
* Unsigned: `uint8`, `uint16`, `uint32`, `uint64`, `uint128`, `uint256`, `uint512`, `uint1024`, `uint2048`, `uint4096`.

### Fixed-Point, Balanced & Rational Types
* `frac8` … `frac64`: True fractional rationals (exact numerator and denominator, zero drift).
* `tbb8` … `tbb256`: Twisted Balanced Binary. Overflows/underflows yield sticky `ERR` rather than trapping. Ideal for continuous control loops, avionics, and DSP.
* `tfp32` … `tfp256`: Fixed-point decimals with deterministic scaling.
* `dim256`: Dimensional analysis scalar carrying physical units.

### Floating-Point & Primitives
* Floats: `flt32`, `flt64`, `flt128`, `flt256`, `flt512` (IEEE 754, produces `inf` / `nan`, no trap).
* Characters: `char8`, `char16`, `char32`.
* Strings: `string` (length-tracked UTF-8 slice), `cstring` (null-terminated C ABI compatibility pointer).
* Ternary & Sub-byte: `trit` ($-1, 0, 1$), `tryte` (9 trits), `nit` (base-3 unit), `nyte`.
* System primitives: `fd`, `pid`, `tid`, `uid`, `gid`, `oflags`, `prot`, `mflags`, `fmode`, `buffer`.

### Containers & Memory Types
* `Result<T>`: Core return container holding `{ is_error: bool, val: T, err: Error }`.
* `Optional<T>`: Nullable container holding `{ has_val: bool, val: T }` or `NIL`.
* `List<T>`: Managed dynamic array.
* `Handle<T>`: Generational reference (`.index: u64`, `.generation: u32`) preventing use-after-free.
* `arena<T>->`: Bump/slab allocated arena memory block.
* `shared_arena<T>`: Thread-safe atomically synchronized arena.
* `atomic<T>`: Native LLVM atomic scalar (`int32` or `bool`). Supports `.load()`, `.store()`, `.swap()`, `.fetch_add()`, `.fetch_sub()`, `.compare_exchange()`.
* `Bridge`: IPC channel handle for out-of-process drivers.
* `OwnedFd`: RAII-managed file descriptor (auto-closes on drop).
* `Duration`: Time delta representation (`duration_secs()`, `duration_millis()`).
* `Path`: Structured filesystem path representation.

---

## 4. Literal Formats & Suffixes (D-147, D-148)

All numeric literals begin with a decimal digit `0`–`9` (the leading-digit rule, D-147). Legacy C prefixes (`0x`, `0b`, `0o`) are invalid. Bases and widths are specified via suffixes:

| Base | Suffix | Digits Allowed | Example with Type Suffix | Decimal Value |
|---|---|---|---|---|
| **Decimal** | *(none)* | `0-9`, `_` | `42i32`, `1000u64` | 42, 1000 |
| **Hexadecimal** | `hex` | `0-9`, `a-f`, `A-F`, `_` | `0FFhexu8`, `0DEADBEEFhexu64` | 255, 3735928559 |
| **Binary** | `bin` | `0-1`, `_` | `1010_1010binu8`, `1111binu16` | 170, 15 |
| **Octal** | `oct` | `0-7`, `_` | `755octu16`, `0755octu32` | 493, 493 |
| **Balanced Ternary** | `tri` / `t` | `0, 1`, `T, t` ($-1$) | `1T01tri`, `1T01t` | 22 |
| **Balanced Nonary** | `non` / `n` | `0-4`, `a-d, A-D` ($-1 \dots -4$) | `1A2non`, `1A2n` | 74 |
| **Floating Point** | *(none)* | `0-9`, `.`, `e/E` | `3.1415f32`, `2.71828f64` | 3.1415, 2.71828 |

> **Leading-Digit Rule:** If a hex literal's first significant digit is a letter `A`–`F`, prepend a value-neutral `0` (e.g. `0FFhex`, never `FFhex`). C-style `0xFF` generates a `NITPICK-LEX-003` error.

---

## 5. Keywords & Modifiers

### Declarations
* `func:name = Ret(Params)`: Function definition.
* `struct:Name = { ... }`: Struct definition.
* `enum:Name = { ... }`: Enum definition.
* `trait:Name = { ... }`: Trait contract definition.
* `impl:Type:Trait = { ... }`: Trait implementation on type.
* `error:Name;`: Custom typed error constant.
* `mod:name;`: Module namespace declaration.
* `extern:"driver"`: Out-of-process IPC bridge interface block.

### Modifiers
* `pub`: Public export visibility.
* `async` / `await`: Asynchronous function / suspension point.
* `inline` / `noinline`: Compiler inlining directives.
* `comptime`: Compile-time evaluation guarantee.
* `sealed`: Prevents downstream struct extension.
* `hidden`: Internal linkage / hidden symbol visibility.
* `fixed`: Immutable compile-time constant binding (has no memory address).
* `stack`: Forces stack allocation without heap escape.
* `defer`: Defers statement execution until function scope exit.

### Safety Escape Hatches (Danger Keywords)
* `raw`: Unwraps value from infallible or unsafe operations.
* `drop`: Discards value or error explicitly.
* `wild`: Unmanaged heap pointer outside RAII tracking. Must be explicitly freed with `dalloc`.
* `wildx`: W^X executable memory page pointer (for JIT engines).

### Formal Verification & Contracts
* `never fails`: Infallibility contract verified by compiler.
* `requires <condition>`: Pre-condition obligation checked by Z3.
* `ensures <condition>`: Post-condition guarantee verified by Z3.
* `invariant <condition>`: Loop/type invariant.
* `acquires <resource>` / `gives <resource>`: Resource ownership contracts.
* `prove <prop>`: Static theorem prover obligation.
* `assert_static <prop>`: Compile-time static assertion.
* `limit<Rules>`: Constrained range type checked after every write.

---

## 6. Built-in Intrinsics

Intrinsics available globally without imports:

### Slab Allocator (`NitpickAlloc`)
* `alloc(size: int64) -> wild int8->`: Allocates uninitialized 16-byte aligned bytes.
* `aalloc(size: int64, align: int64) -> wild int8->`: Power-of-two aligned allocation.
* `calloc(count: int64, size: int64) -> wild int8->`: Zero-initialized allocation.
* `ralloc(ptr: wild any->, new_size: int64) -> wild int8->`: Resizes allocation.
* `dalloc(ptr: wild any->) -> NIL`: Frees memory. Traps on foreign/NULL pointer.
* `mcpy(dst, src, n)` / `mmov(dst, src, n)`: Non-overlapping / overlapping memory copies.
* `memset(dst, val, n)`: Memory fill.

### Specialized Intrinsics
* `arena_make(cap: int64) -> arena<T>`: Creates bump arena for type `T`.
* `shared_arena_make(cap: int64) -> shared_arena<T>`: Creates thread-safe shared arena.
* `atomic_from_ptr::<T>(ptr: wild T->) -> atomic<T>`: Aliases address as atomic without allocation.
* `wild_live_count() -> int64`: Returns count of currently active `wild` allocations.
* `clone_exec(blk: wild any->) -> Result<int64>`: Supervised child process spawn.
* `driver_retire(slot: int64) -> NIL`: Frees supervisor registry slot.
* `wildx_alloc(size)` / `wildx_seal(ptr)` / `wildx_call(ptr, arg)` / `wildx_free(ptr)`: W^X JIT execution primitives.
