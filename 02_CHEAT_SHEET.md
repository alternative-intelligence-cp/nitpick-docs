# Nitpick cheat sheet

Examples for the [quick start](01_QUICK_START.md). These are expressions, statements and declarations to place in complete programs.
A program may need more `failsafe` arms than the quick start. The compiler names each missing arm; the relevant rows name common extras.
For `./lib/...` imports, save the source beside `lib/` at the compiler clone root, or adjust the path.


## Bindings and calls

| Example | Meaning |
|---|---|
| `int32:n = 3i32; n = 4i32;` | Type, name, initial value; then assignment. |
| `fixed int64:n = argv.len;` | A write-once binding; its initial value may be computed at runtime. |
| `func:f = int32() never fails { pass 7i32; };` | A function returning a successful Result<int32>; never fails declares that it does not return an error. |
| `f()` | Call a function with no arguments. |
| `..*int32[]:xs; sum(1i32, 2i32)` | A variadic parameter collects arguments into a slice. |
| `sum(..^xs)` | Spread a slice into variadic arguments. |
| `7i32 => int64; 257i32 =>! uint8` | A widening cast and a truncating cast. |
| `int32:_~arg` | A discarded parameter; reading it in the body is a compile error. |
| `async func:later = int32() { pass 7i32; }; await later()` | Declare an async function; await its Result. |
| `a[0i64...2i64]` | Slice containing the first two array elements. |
| `a[0i64]` | Index the first array element. |

## Operators and precedence

| Example | Meaning |
|---|---|
| `7i32 + 2i32; 7i32 - 2i32; 7i32 * 2i32` | Addition, subtraction and multiplication. Integer overflow reaches failsafe as IntOverflow. |
| `7i32 / 2i32; 7i32 % 2i32` | Integer quotient 3; remainder 1. May need DivByZero and DivOverflow arms. |
| `255u8 +% 1u8; 0u8 -% 1u8; 16u8 *% 16u8` | Wrapping addition, subtraction and multiplication. |
| `-3i32; !false; ~0u8` | Negation, logical NOT, bitwise NOT. |
| `8u32 << 1u32; 8u32 >> 1u32` | Left and right bit shifts; a computed out-of-range shift reaches ShiftRange. |
| `6u32 & 3u32; 6u32 ^ 3u32; 6u32 \| 3u32` | Bitwise AND, XOR and OR. |
| `3i32 < 4i32; 3i32 <= 3i32; 4i32 > 3i32; 4i32 >= 4i32` | Boolean comparisons. |
| `3i32 == 3i32; 3i32 != 4i32` | Equality and inequality. |
| `3i32 <=> 4i32` | Three-way comparison: -1 for less, 0 for equal, 1 for greater. |
| `false && raw bump(@n); true \|\| raw bump(@n)` | Short-circuiting logical AND and OR. |
| `#size_of<int32>` | The type's size in bytes. |
| `x += 2i32; x -= 1i32; x *= 2i32; x /= 3i32; x %= 5i32;` | Add, subtract, multiply, divide or take remainder, then assign. May need IntOverflow, DivByZero and DivOverflow arms. |
| `x +%= 1i32; x -%= 1i32; x *%= 2i32;` | Wrapping arithmetic, then assignment. |
| `x &= 7i32; x \|= 8i32; x ^= 1i32;` | Bitwise AND, OR or XOR, then assignment. |
| `x <<= 1i32; x >>= 1i32;` | Shift left or right, then assign. May need ShiftRange. |

Precedence, highest to lowest, from `meta/specs/OP_REFERENCE.md` §0. Result unary associates right. Assignment is a statement.
Level numbers are the reference's; its level 3 no longer exists.
Parentheses make grouping explicit.

| Level | Operators |
|---|---|
| 1 Postfix | `() [] . ?.` |
| 2 Result unary | `raw _! drop _? await relay _^` |
| 4 Cast | `=> =>!` |
| 5 Unary | `! ~ - @ <- $$i $$m` |
| 6 Multiplicative | `* / % *%` |
| 7 Additive | `+ - +% -%` |
| 8 Shift | `<< >>` |
| 9 Range / spread | `.. ... ..* ..^` |
| 10 Relational | `< <= > >= <=>` |
| 11 Equality | `== !=` |
| 12 Bitwise AND | `&` |
| 13 Bitwise XOR | `^` |
| 14 Bitwise OR | `\|` |
| 15 Logical AND | `&&` |
| 16 Logical OR | `\|\|` |
| 17 Null coalescing | `?? ??!` |
| 18 Conditional / fallback | `is` `?\|` |
| 19 Assignment | `= += -= *= /= %= &= \|= ^= <<= >>=` |

## Control flow

| Example | Meaning |
|---|---|
| `if (flag) { ... } else { ... }` | Selects a branch. |
| `for (int32:i in 1i32..3i32)` | Inclusive range: 1, 2, 3. |
| `for (int32:i in 0i32...3i32)` | Half-open range: 0, 1, 2. |
| `pick (n) { (6i32) { ... }, (*) { ... } }` | Match 6, otherwise the wildcard. |
| `defer { x = 7i32; }` | Run a deferred block when its enclosing scope exits. |

## Results and optionals

| Example | Meaning |
|---|---|
| `Result<int32>:r = find(true); r.is_error; r.value` | Test for an error before reading the value. |
| `fail MissingItem; r.err` | Fail with a named error; r.err reads the error of a failed Result. |
| `find(false) ?\| 12i32` | Fallback value 12. |
| `relay find(present); _^ find(present)` | Yield the successful value, or return the same error to the caller. |
| `raw answer(0i32); _! answer(0i32)` | Take the value from a never fails Result; long and short spellings. |
| `drop note(); _? note();` | Discard a never fails NIL result; long and short spellings. |
| `discard(raw answer(0i32)); _~ raw f(0i32);` | Explicitly discard a value. |
| `int32?:n = NIL; n == NIL` | An optional with no value; compare with NIL to test for absence. |
| `n ?? 9i32` | An optional value or the fallback when empty. |
| `find(false) ?! E9` | Send E9 to failsafe on failure. |
| `!!! E9;` | Send the named error directly to failsafe. |
| `o?.x` | Read a stored field through an optional; the field result is optional. |

## Types

| Example | Meaning |
|---|---|
| `int8, int16, int32, int64, int128, int256, int512, int1024, int2048, int4096` | Signed integer types with explicit bit widths. |
| `uint8, uint16, uint32, uint64, uint128, uint256, uint512, uint1024, uint2048, uint4096` | Unsigned integer types with explicit bit widths. |
| `flt32, flt64, flt128` | Floating-point types; sizes 4, 8 and 16 bytes. |
| `tfp32, tfp64, tfp128, tfp256` | Fixed-point numeric types. Operations may need a TbbErr arm. |
| `dim256<Meters>:d = 2.0dim256<Meters>;` | A dimensioned value with a unit parameter. |
| `frac8, frac16, frac32, frac64` | Fractional numeric types. |
| `tbb8, tbb16, tbb32, tbb64, tbb128, tbb256` | Twisted balanced binary numeric types. |
| `char8, char16, char32` | Character types. |
| `trit, tryte, nit, nyte; tryte:n = 29524;` | Ternary and nonary integer types; tryte is a balanced-ternary integer. |
| `string:s = "hello"; s.len; cstring:c = "hello";` | A string with a length field, and a C-compatible string. |
| `Duration:d = raw duration_ms(3i64); raw duration_secs(2i64)` | Construct durations from milliseconds or seconds; .ns is the duration in nanoseconds. |
| `bool` | The type of true and false. |
| `Result<T>` | A success value or an error; test .is_error before reading .value, and use .err for an error. |
| `T?` | An optional value of T, or NIL. |
| `int32[3]:a = [1i32, 2i32, 3i32];` | An array of three int32 values; a[1i64] is 2. |
| `fd` | A file descriptor. |
| `pid, tid` | Process and thread identifiers. |
| `uid, gid` | User and group identifiers. |
| `oflags` | File-opening flags. |
| `prot` | Memory-protection flags. |
| `mflags` | Memory-mapping flags. |
| `fmode` | File permission bits. |
| `buffer` | An owning byte buffer. |
| `OwnedFd:o = own_fd(n);` | A file descriptor owner. |
| `Path:p = path_parse("/") ?! E9;` | A parsed filesystem path. |

## Literals

| Example | Meaning |
|---|---|
| `42i32; 1000u64` | Decimal integer literals. |
| `0FFhexu8; 0DEADBEEFhexu64; 0FFhex` | Hexadecimal: 255, 3735928559, 255. Start with a decimal digit and use a base suffix. |
| `1010_1010binu8; 1111binu16; 1010binu8` | Binary: 170, 15, 10. |
| `755octu16; 0755octu32` | Octal: both are 493. |
| `1T01tri; 1T01t` | Long and short balanced-ternary suffixes: both are 19. |
| `1A2non; 1A2n` | Long and short balanced-nonary suffixes: both are 74. |
| `3.1415f32; 2.71828f64` | Floating-point literals with width suffixes. |

## Declarations and modules

| Example | Meaning |
|---|---|
| `struct:Point = { int32:x; int32:y; };` | Fields declared inside a struct. |
| `enum:Color = { Red; Blue; };` | An enum with named variants. |
| `trait:Read = { func:read = int32(Self:self) never fails; };` | A trait declares a method signature. |
| `impl:Point:Read = { func:read = int32(Point:self) never fails { pass self.x; }; };` | An implementation supplies a trait method for a type. |
| `error:MissingItem;` | A named error declaration. |
| `struct:Box<T> = { T:value; }; func:id<T: Copy> = T(T:x) never fails { pass x; };` | A generic struct and a function with a trait bound. |
| `use "./helpers.npk".*;` | Import public declarations from a source file, using a path relative to this file. |
| `inline func:a = int32() never fails { pass 1i32; }; noinline func:b = int32() never fails { pass 2i32; };` | Inlining modifiers on a function; the reference files don't describe their effect yet. |
| `comptime func:f = int32() never fails { pass 7i32; }; comptime(raw f())` | A comptime function and a compile-time call. |
| `struct:S = { sealed int32:x; };` | A field readable outside its declaring module, but writable only inside it (TYPE_REFERENCE.md §9.1.1). |
| `hidden int32:x;` | A struct field that code outside its declaring module cannot read or write (TYPE_REFERENCE.md §9.1.1). |
| `stack int32[2]:a = [1i32, 2i32];` | Asks for the stack memory space for this local; see `meta/specs/MEMORY_REFERENCE.md`. |
| `mod:example;` | Name this source module example; save it as example.npk. |
| `mod:helpers = { pub func:f = int32() never fails { pass 7i32; }; };` | An inline module; pub makes f available to importers. |

## Pointers and borrows

| Example | Meaning |
|---|---|
| `int32->:p = @x; <-p` | Address of x; read its value. |
| `p.x; p.read()` | Field access and method call through a struct pointer. |
| `int32->:p = $$i x;` | A shared borrow of a value. |
| `int32->:p = $$m x;` | An exclusive borrow of a value. |

## Allocation and containers

| Example | Meaning |
|---|---|
| `List<int32>:xs = raw list_init::<int32>(4i64);` | A list with an element type and initial capacity. May need LimitViolated and DecreasesViolated. |
| `Handle<int64>:h = a.alloc(7i64);` | A handle returned by shared-arena allocation; may need StaleHandle. |
| `arena<int64>:a = arena_make(4i64); arena<int64>->:p = @a;` | An arena and its address; create it with an element type and capacity. May need StaleHandle. |
| `shared_arena<int64>:a = shared_arena_make(4i64);` | A shared arena with an element type and capacity. May need StaleHandle. |
| `atomic<int32>; .load(); .store(v); .swap(v); .fetch_add(v); .fetch_sub(v); .compare_exchange(old, new)` | An atomic scalar and its read, write, exchange and arithmetic operations. |
| `wild int8->:p = alloc(8i64); dalloc(p);` | Allocate a byte block, hold its wild pointer, then release it. |
| `wildx uint8->:p = wildx_alloc(16i64) =>! wildx uint8->; wildx_seal(p); wildx_call(p, arg); wildx_free(p);` | Allocate executable memory, seal it, call it, then release it; the call requires machine code and an int64 argument. |
| `aalloc(16i64, 32i64)` | Allocate a byte block with an alignment argument. |
| `calloc(4i64, 8i64)` | Allocate a zero-filled block with count and element size. |
| `ralloc(p, 32i64)` | Resize a byte allocation. |
| `mcpy(dst, src, n); mmov(dst, src, n);` | Copy bytes between blocks or move bytes within a block. |
| `memset(p, 7i64, 8i64)` | Fill a byte block. |
| `atomic_from_ptr::<int64>(p).store(7i64); atomic_from_ptr::<int64>(p).load()` | Use an atomic view of a pointer as a method receiver. |
| `wild_live_count()` | The current count of wild allocations. |

## Contracts

| Example | Meaning |
|---|---|
| `requires n > 0i32` | At run time a false precondition reaches failsafe as RequiresViolated; name that arm. |
| `ensures result == old(n)` | Check the successful result against the postcondition; false reaches failsafe as EnsuresViolated. old(n) is n at entry. |
| `while (n < 3i32) decreases 3i32 - n invariant n >= 0i32` | A loop with a decreasing expression and an invariant; may need DecreasesViolated and InvariantViolated. |
| `acquires 2i32; gives` | Concurrency contracts: `acquires N` marks a function as acquiring a lease; `gives` marks a factory whose return hands its channels to the caller. See `meta/specs/CONCURRENCY_REFERENCE.md`. |
| `prove(1i32 == 1i32);` | With --verify, Z3 checks a compile-time obligation under path conditions; compilation fails if it finds a counterexample. A plain build lowers it to nothing. |
| `assert_static(1i32 == 1i32);` | Evaluate a constant condition at compile time; a false condition stops compilation. |
| `Rules<int64>:small = { $ >= 0i64, $ <= 100i64 }; limit<small> int64:n = 7i64;` | Name a rule and apply it to a binding; violations can need a LimitViolated arm. |

## Driver calls

| Example | Meaning |
|---|---|
| `use "./lib/nbridge.npk".*; Bridge->:b; extern:"driver" = { func:ping = int64(Bridge->:b, int64:v, Duration:within); };` | Import Bridge and declare a driver interface; see the [driver guide](03_DRIVER_MODEL_GUIDE.md) for placement, C driver and build. Add the nbridge arms listed below. |
| `clone_exec(p)` | Launch a child using a parameter block; see the [driver guide](03_DRIVER_MODEL_GUIDE.md#low-level-driver-calls) for types and context. |
| `driver_retire(slot);` | Retire a child registry slot after reaping; see the [driver guide](03_DRIVER_MODEL_GUIDE.md#low-level-driver-calls) for types and context. |

The nbridge import/interface rows may also need `nbridge.EDriverSpawn`, `nbridge.EDriverFault`, `nbridge.EDriverProtocol`, `nbridge.EDriverDeadline`, `nbridge.EDriverError`, `nbridge.ERingFull`, `nbridge.EBridgePoisoned`, `nbridge.EShmCreate`, `nbridge.EShmSeal`, `nbridge.EShmMap`, `nbridge.DeadlineExceeded`, `nbridge.DriverLeak` and `BadPath`. Use the names the compiler reports; the complete driver program shows its imported names.

Numeric literals use base suffixes. The prefixes `0x`, `0b` and `0o` are refused with `NITPICK-LEX-003`.

Further reading: `meta/specs/MEMORY_REFERENCE.md` and `meta/specs/VERIFICATION_REFERENCE.md`.
For types, operators and literals: `meta/specs/TYPE_REFERENCE.md`, `meta/specs/OP_REFERENCE.md` and `meta/specs/LEXICAL_REFERENCE.md`.
For declarations and calls: `meta/specs/MODULE_REFERENCE.md`, `meta/specs/TRAITS_REFERENCE.md`, `meta/specs/CONTROL_REFERENCE.md` and `meta/specs/BUILTIN_REFERENCE.md`.

Checked against compiler commit 64861f8.
