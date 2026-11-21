---
name: cpp-memory-leak-prevention
description: Prevent, detect, and fix memory and resource-safety bugs in C++ projects — memory leaks, use-after-free, buffer overflows, dangling string_view/references/lambda captures, iterator invalidation, uninitialised data, signed overflow and narrowing conversions, object slicing, and data races. Use this skill whenever the user writes, reviews, refactors, or debugs C++ involving new/delete, raw pointers, ownership, smart pointers, RAII, containers of pointers, callbacks capturing shared_ptr, string_view/span lifetimes, casts between numeric types, threads, or C APIs (malloc/free, fopen, OpenCV, Qt, COM, OpenCascade). Also use it when the user mentions memory leak, valgrind, AddressSanitizer, LeakSanitizer, UBSan, ThreadSanitizer, heaptrack, crash/segfault, dangling pointer, double free, circular references, undefined behavior, C++ contracts, or asks to make C++ safe, harden it, or modernize legacy C++ — even if they never say 'leak' explicitly.
---

# C++ Memory & Resource Safety (leaks first, then the rest)

Goal: make leaks and lifetime bugs structurally impossible through ownership design (RAII), then catch what remains with sanitizers and static analysis in CI. Prefer *prevention by design* over *detection after the fact*.

## Which defect → which detector → which reference

| Defect | Typical symptom | Detector (verified) | Fix / reference |
|---|---|---|---|
| Leak | RSS grows; LSan report | ASan+LSan | Ownership Rules below; `references/patterns.md` |
| Use-after-free, dangling `string_view`/lambda ref, iterator invalidation | Crash, corrupted data | ASan (`detect_stack_use_after_return=1` for returned lambdas) | `references/memory-safety-errors.md` §1,3,4 |
| Out-of-bounds | Crash, security bug | ASan; `-D_GLIBCXX_ASSERTIONS` | §2 |
| Signed overflow, null deref | Works until optimiser changes | UBSan **with `-fno-sanitize-recover=all`** | §5 |
| Uninitialised data | Flaky, build-dependent | `-ftrivial-auto-var-init=pattern`, clang-tidy member-init, MSan/Valgrind | §6 |
| Narrowing / float→int cast (Ariane 5 class) | Silent wrong values | `-Wconversion -Werror=narrowing`, `-fsanitize=float-cast-overflow` | §7 |
| Slicing | Wrong virtual call | Test; clang-tidy `cppcoreguidelines-slicing` | §8 |
| Data race | Heisenbugs | TSan (separate build) | §9 |

## Workflow

1. **Classify the task**
   - Writing new code → apply the Ownership Rules below while writing.
   - Reviewing/refactoring existing code → run the Review Checklist, then propose minimal diffs.
   - Debugging a known leak → go to `references/debugging-tools.md`, then fix the root cause.
   - Crash/UB/lifetime/numeric/thread bug (not a leak) → `references/memory-safety-errors.md`.
2. **Identify the C++ standard** (check CMakeLists `CMAKE_CXX_STANDARD`, compiler flags). Default to C++17; use `std::make_unique` (C++14+), avoid C++20-only features unless confirmed.
3. **Apply rules, explain each change briefly** (what leaked / could leak, why the new form can't).
4. **Add or recommend tooling** (sanitizers, clang-tidy) using `references/build-and-ci.md` if the project lacks it.

## Ownership Rules (the core)

1. **Every resource has exactly one owner, expressed in the type.** Heap memory, file handles, sockets, mutexes, GPU/GL handles, and C-API objects all follow this.
2. **No naked `new` / `delete` / `malloc` / `free` in application code.**
   - Single object: `std::make_unique<T>(...)`
   - Shared ownership (only when truly needed): `std::make_shared<T>(...)`
   - Arrays/buffers: `std::vector<T>`, `std::array<T,N>`, `std::string`; if unavoidable, `std::unique_ptr<T[]>`.
3. **Raw pointers and references mean "non-owning observer" only.** Never `delete` through them. If a function takes ownership, take `std::unique_ptr<T>` by value.
4. **Default to `unique_ptr`; reach for `shared_ptr` last.** `shared_ptr` hides lifetime and enables cycles.
5. **Break cycles with `weak_ptr`.** Parent → child owns (`shared_ptr`/`unique_ptr`); child → parent observes (`weak_ptr` or raw non-owning pointer). Lock `weak_ptr` before use.
6. **Wrap C resources in RAII at the boundary**, immediately after acquisition:
   ```cpp
   struct FileCloser { void operator()(FILE* f) const noexcept { if (f) std::fclose(f); } };
   using FilePtr = std::unique_ptr<FILE, FileCloser>;
   FilePtr f{std::fopen(path, "rb")};
   ```
   Generic alternative: a small hand-written `ScopeGuard` or `gsl::finally`. (`std::experimental::scope_exit` exists only in some standard libraries; don't rely on it for portable code.)
7. **Rule of Zero.** Let members manage themselves; write no destructor/copy/move. If you must manage a resource directly, follow the **Rule of Five** (destructor, copy ctor/assign, move ctor/assign — define, `= default`, or `= delete` all five) and make moves `noexcept`.
8. **Base classes with virtual functions get a `virtual` destructor** (or protected non-virtual if not deleted polymorphically). Missing this leaks the derived part.
9. **Exception safety:** acquire → wrap in RAII in the same statement. Prefer `make_unique` over `f(std::unique_ptr<T>(new T), g())`. (That call form can leak in C++11/14 if `g()` throws between `new` and the `unique_ptr` constructor; C++17 forbids interleaving argument evaluation, so it is safe there — but `make_unique` is still cleaner and `modernize-make-unique` flags it.)
10. **Containers of owning raw pointers are banned.** Use `std::vector<std::unique_ptr<T>>`.
11. **Callbacks/lambdas:** capturing `shared_ptr` of `this` in something `this` stores (signal, timer, thread) creates a cycle. Capture `weak_ptr` (`weak_from_this()` needs C++17; use `std::weak_ptr<T>(shared_from_this())` on C++11/14) and lock inside, or capture a raw `this` only if the lifetime is provably longer.
12. **Threads/async:** join or detach deliberately; use `std::jthread` (C++20 only) or a joining wrapper on older standards so threads never outlive the data they reference.
13. **Factory/ownership transfer APIs return `unique_ptr`**, not raw pointers; "out" parameters of owning raw pointers (`T**`) are replaced by return values.

## Review Checklist

When reviewing code, scan for and report each hit with file:line and a fix:

- [ ] `new` without an immediate owner; `delete`/`delete[]` anywhere outside a deleter
- [ ] `malloc/calloc/realloc/free`, `fopen/fclose`, `CreateX/CloseHandle`, `glGen*/glDelete*` not wrapped in RAII
- [ ] Early `return`, `throw`, `break`, `continue`, `goto` between acquire and release
- [ ] Functions returning or taking owning raw pointers
- [ ] `shared_ptr` members forming parent↔child or observer↔subject cycles
- [ ] Lambdas capturing `shared_ptr<this>` / `shared_from_this()` stored long-term
- [ ] Classes with raw owning members but no (or partial) Rule of Five
- [ ] Polymorphic base without virtual destructor
- [ ] `static`/global containers that only grow (caches, registries, listener lists without removal) — **logical leaks** that sanitizers don't flag
- [ ] `std::vector<T*>`, `std::map<K, T*>` owning heap objects
- [ ] `reinterpret_cast`/`void*` hiding ownership
- [ ] Hand-written RAII types whose **move assignment** doesn't release the old resource first, or whose moved-from state isn't explicitly emptied (`std::exchange`) — defaulted moves of `std::function`/handles are a common source
- [ ] `std::thread` stored without join; detached threads touching freed data
- [ ] `new T[n]` inside loops without a matching owner; `unique_ptr::release()` whose result is dropped

## Logical leaks (RAII won't save you)

Memory still referenced but never used: unbounded caches, event listeners never unsubscribed, maps keyed by IDs never erased, `shared_ptr` cycles, per-request objects pushed into global lists. Recommend: bounded caches (LRU), explicit unsubscribe tokens (RAII `Connection` objects), `weak_ptr` observers, and periodic size metrics/logging for long-running services.

## Refactoring pattern (legacy → safe)

Make small, behavior-preserving steps and keep the build green:

1. Turn on sanitizers and get a baseline (see `references/build-and-ci.md`).
2. Replace owning raw members with `unique_ptr`; fix compile errors by making ownership explicit.
3. Replace `T**` out-params / raw returns with `unique_ptr` returns.
4. Replace manual arrays with `std::vector`/`std::array`.
5. Delete now-empty destructors (Rule of Zero).
6. Re-run sanitizers and tests.

See `references/patterns.md` for before/after examples of each.

## Output expectations

- Show **minimal diffs** or before/after snippets, not full-file rewrites, unless asked.
- State the leak scenario in one sentence per finding (e.g., "exception thrown by `parse()` skips `delete buf`").
- Do not introduce `shared_ptr` just to silence ownership questions; say who the owner is.
- If a third-party API forces raw ownership (e.g., Qt parent-child, which deletes children), respect the framework's model rather than wrapping it in `unique_ptr` (double delete risk). Note this explicitly.
- If you cannot run code, say which sanitizer command the user should run to confirm.

## Verify, don't assume

Whenever you produce or change C++ to fix a leak and can run code, verify deterministically instead of asserting it is fixed:

```bash
bash scripts/asan_check.sh path/to/file.cpp [extra g++ flags]   # ASan+UBSan(no-recover)+LSan, exit 0 = clean
# data races: g++ -std=c++17 -g -O1 -fsanitize=thread -pthread file.cpp && ./a.out
```
Limits to remember (verified): LeakSanitizer does **not** report unclosed `fopen`/`open` handles (glibc keeps live `FILE`s reachable) — check fds with `valgrind --track-fds=yes` or `lsof`. Static analysis (`clang-analyzer-cplusplus.NewDeleteLeaks`) catches simple early-return leaks but missed an exception-path leak; ASan/LSan at runtime caught both. A `shared_ptr` cycle is *not* flagged by LSan if a `weak_ptr` or global still points into it — use a `weak_ptr::expired()` assertion.

`scripts/tests/` holds paired BAD/GOOD snippets and three runners: `run_evals.py` (leak patterns), `run_memsafety.py` (UAF, bounds, dangling, UB, uninit, narrowing, slicing, races) and `run_tools.py` (check_all layers, fuzzing, CBMC). Both prove each pattern behaves as the references claim. Gotcha: UBSan exits 0 on errors unless built with `-fno-sanitize-recover=all`.

## Tooling layers (one command)

```bash
bash scripts/check_all.sh file.cpp [--msan]            # warnings, ASan+UBSan, TSan*, cppcheck, clang-tidy, Valgrind, MSan (opt-in); *only if threads used
bash scripts/fuzz_run.sh target.cpp 60 corpus/         # libFuzzer+ASan+UBSan for code that handles untrusted input (see scripts/fuzz_template.cpp)
bash scripts/cbmc_check.sh harness.c                   # bounded model checking, e.g. prove a float->int conversion cannot overflow
```
Use `check_all.sh` after any fix when you can run code; report which layers PASS/FAIL/SKIP. Read `references/tooling.md` for what each tool misses (verified: LSan misses fd leaks, cppcheck missed variable-index OOB and signed overflow, Valgrind missed a returned dangling lambda reference). Recommend fuzzing for parsers/deserialisers and CBMC for numeric-range or small safety-critical kernels. `pip install cbmc` is the wrong package; use `apt install cbmc`.

## Reference files

- `references/patterns.md` — before/after code for common leak patterns (exceptions, cycles, C APIs, callbacks, containers).
- `references/debugging-tools.md` — ASan/LSan/Valgrind/heaptrack/MSVC CRT/Instruments usage and how to read reports.
- `references/memory-safety-errors.md` — non-leak defects (UAF, bounds, dangling views, UB, narrowing/Ariane 5 lessons, slicing, races, C++26 contracts) with tested fixes.
- `references/tooling.md` — coverage matrix of ASan/TSan/Valgrind/cppcheck/clang-tidy/MSan with verified blind spots; fuzzing and CBMC recipes.
- `references/build-and-ci.md` — CMake snippets, clang-tidy checks, CI job, and test patterns for leak regression.
