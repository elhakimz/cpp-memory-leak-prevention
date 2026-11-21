# Non-Leak Defects: Detection and Fixes

Every BAD/GOOD pair below is exercised by `python3 scripts/tests/run_memsafety.py` (g++ 13, ASan/UBSan/TSan). Case ids in brackets.

## Contents
1. Use-after-free / dangling pointers [d01]
2. Out-of-bounds access [d02, d03]
3. Dangling views and references [d04, d05]
4. Iterator/reference invalidation [d06]
5. Signed overflow and other UB [d07, d08]
6. Uninitialised data [d09]
7. Narrowing conversions (the Ariane 5 class) [d10]
8. Object slicing [d11]
9. Data races [d12]
10. Contracts and fail-safe design (lessons from Ariane 5)

## 1. Use-after-free [d01]
```cpp
int* p = new int(5); delete p; use(*p);          // BAD: heap-use-after-free
auto p = std::make_unique<int>(5); int v = *p;   // GOOD: copy the value out; never keep raw aliases past the owner
```
Detector: ASan. Prevention: single owner (`unique_ptr`), raw pointers only as short-lived observers.

## 2. Out-of-bounds [d02, d03]
```cpp
v[i];                       // BAD when i >= size(): heap-buffer-overflow (ASan), silent otherwise
v.at(i);                    // GOOD where failure is recoverable: throws std::out_of_range
char buf[8]; strcpy(buf, s);   // BAD: stack-buffer-overflow
std::string s = src;           // GOOD: owns its storage
```
For code that keeps `operator[]` for speed, build with `-D_GLIBCXX_ASSERTIONS` so out-of-range aborts deterministically [d02_hardened_bad]. Ban `strcpy/sprintf/gets`; prefer `std::string`, `std::span` (pass size with pointer), `snprintf`.

## 3. Dangling views and references [d04, d05]
```cpp
std::string_view sv; { std::string s(100,'x'); sv = s; } sv[0];       // BAD: heap-use-after-free
auto make(){ int local=42; return [&local]{ return local; }; }       // BAD: stack-use-after-return
auto make(){ int local=42; return [local]{ return local; }; }        // GOOD: capture by value
```
Rules: `string_view`/`span` parameters are fine, but never store them beyond the owner's life; never return a lambda capturing locals by reference; never bind a returned view to a temporary. ASan only catches stack-use-after-return with `ASAN_OPTIONS=detect_stack_use_after_return=1` (the test runner sets it). Stack-use-after-scope [d05b] is on by default.

## 4. Iterator invalidation [d06]
```cpp
for (auto& x : v) if (x == 1) v.push_back(9);   // BAD: reallocation invalidates the loop's iterators (ASan: heap-use-after-free)
// GOOD: collect, then mutate after the loop (or use index loops / erase-remove / std::erase_if)
```

## 5. Signed overflow, null dereference [d07, d08]
```cpp
int x = INT_MAX; x += n;                                   // BAD: UB (UBSan: signed integer overflow)
int r; if (__builtin_add_overflow(x, n, &r)) handle();     // GOOD (GCC/Clang); portable: check `x > INT_MAX - n` first
```
Unsigned wrap-around is defined (not UB) but is a logic bug when unintended. Replace nullable raw pointers with `std::optional`/references where "absent" is valid [d08]. **UBSan must be built with `-fno-sanitize-recover=all` or it exits 0 on UB.**

## 6. Uninitialised data [d09]
- GCC's `-Wmaybe-uninitialized` did **not** fire on the classic `int x; if (c) x=1; return x;` at `-O2` (verified, GCC 13) — don't rely on warnings alone. `-Wall -Werror` does catch the *definite* case [d09_definite_bad].
- Deterministic test: build with `-ftrivial-auto-var-init=pattern`; use of an uninitialised local then yields a loud, repeatable wrong value [d09_uninit_bad].
- Members: `clang-tidy -checks=cppcoreguidelines-pro-type-member-init` flags constructors that leave members uninitialised [d09_member_bad]. Fix with default member initialisers (`int port{8080};`).
- Runtime detectors: MemorySanitizer (clang only), Valgrind `--track-origins=yes`.

## 7. Narrowing conversions [d10] — the Ariane 5 failure class
Ariane 5 Flight 501 converted a 64-bit float to a 16-bit signed integer; the value exceeded the range and raised an unhandled operand error (Ada). In C++ the same code is **undefined behaviour** with no exception at all:
```cpp
short s = d;                          // -Wconversion: "may change value" [d10_conv_bad]; silent without the flag
short s{d};                           // GCC: only a warning unless -Werror=narrowing [d10_brace_bad]; clang: error
static_cast<int16_t>(70000.0);        // UB; -fsanitize=float-cast-overflow reports it [d10_cast_ub_bad]
```
Checked conversion (tested against boundaries 32767/32768, -32768/-32769, NaN, 1e300, 2^63, unsigned and int64→int32 cases [d10_narrow_good]): see `scripts/tests/src/d10_narrow_good.cpp` for `checked_narrow<To>(From)`. Key detail: float→int bounds use an *exclusive* upper bound `ldexp(1, digits)`; comparing against `float(max)` is wrong for 64-bit targets because `max+1` isn't representable in `double`. `gsl::narrow` is a ready-made alternative.

## 8. Object slicing [d11]
```cpp
std::vector<Base> v; v.push_back(Derived{});          // BAD: Derived part is sliced off; virtual calls hit Base
std::vector<std::unique_ptr<Base>> v;                 // GOOD
```
Make polymorphic bases non-copyable (`Base(const Base&) = delete;` plus a virtual `clone()` if copies are needed). clang-tidy: `cppcoreguidelines-slicing`.

## 9. Data races [d12]
```cpp
int counter; /* two threads: ++counter */      // BAD: TSan "data race"
std::atomic<int> counter{0};                   // GOOD (or guard with a mutex)
```
Run TSan in its own build (cannot be combined with ASan). Prefer message passing / immutable shared data; lock with `std::scoped_lock`; join threads before the data they touch dies (`jthread` in C++20).

## 10. Contracts and fail-safe design (Ariane 5 lessons)
The post-mortem pointed at three process failures that apply to any language:
1. **Unstated assumptions on reuse.** Ariane 4's range analysis justified leaving a conversion unprotected; nobody re-checked it for Ariane 5. Write assumptions as executable preconditions: `assert`, `static_assert`, `checked_narrow`, `gsl::Expects`, or in **C++26** the standard `pre(...)`, `post(...)`, `contract_assert(...)` (standardised March 2026; GCC 16 implements them via `-std=c++26 -fcontracts`; enforcement semantics are chosen at build time — **not verified in this skill's sandbox, which has GCC 13**; check your compiler's docs for flags).
2. **Common-mode failure.** Two identical units running identical code failed identically. Redundancy needs diversity or independent validation, not copies.
3. **Wrong failure response.** Shutting down a flight-critical unit on an exception and emitting diagnostic data as if it were flight data was fatal. Decide per component what "safe failure" means (saturate, hold last good value, switch to backup) and test that path.
Also: dead/unneeded code that can fail is a liability — Ariane's alignment function was not even needed after liftoff.
