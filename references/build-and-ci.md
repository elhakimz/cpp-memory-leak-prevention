# Build Config, Static Analysis, CI

## Contents
- CMake sanitizer option
- clang-tidy checks
- Compiler warnings
- CI job
- Leak regression tests

## CMake
```cmake
option(ENABLE_SANITIZERS "ASan+UBSan" OFF)
if(ENABLE_SANITIZERS AND NOT MSVC)
  add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=address,undefined -fno-sanitize-recover=all)
endif()
```
Run: `cmake -B build-asan -DENABLE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build-asan && ctest --test-dir build-asan --output-on-failure`

Note: do not combine ASan with TSan; run them as separate builds. Don't use ASan with Valgrind.

## clang-tidy (.clang-tidy)
```yaml
Checks: >
  -*,
  cppcoreguidelines-owning-memory,
  cppcoreguidelines-no-malloc,
  cppcoreguidelines-special-member-functions,
  cppcoreguidelines-virtual-class-destructor,
  modernize-make-unique,
  modernize-make-shared,
  modernize-use-nullptr,
  modernize-use-equals-delete,
  bugprone-unique-ptr-array-mismatch,
  bugprone-use-after-move,
  bugprone-shared-ptr-array-mismatch,
  bugprone-unhandled-self-assignment,
  clang-analyzer-cplusplus.NewDelete,
  clang-analyzer-cplusplus.NewDeleteLeaks,
  misc-new-delete-overloads
WarningsAsErrors: 'cppcoreguidelines-owning-memory,clang-analyzer-cplusplus.NewDeleteLeaks'
```
`cppcoreguidelines-owning-memory` needs `gsl::owner<T*>` annotations (GSL) to be most useful; it still flags naked new/delete.

## Warnings
GCC/Clang: `-Wall -Wextra -Wnon-virtual-dtor -Wdelete-non-virtual-dtor -Woverloaded-virtual -Wshadow`
MSVC: `/W4 /analyze`

## GitHub Actions example
```yaml
name: memory-safety
on: [push, pull_request]
jobs:
  asan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: cmake -B build -DENABLE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
      - run: cmake --build build -j
      - run: ctest --test-dir build --output-on-failure
        env:
          ASAN_OPTIONS: detect_leaks=1:abort_on_error=1
  tidy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: sudo apt-get install -y clang-tidy
      - run: cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
      - run: run-clang-tidy -p build
```

## Leak regression tests
- Run the whole test suite under ASan in CI; LSan makes any leaking test fail.
- For cycle-prone types, assert lifetime with `weak_ptr`:
```cpp
TEST(Graph, NoCycleLeak) {
  std::weak_ptr<Node> w;
  { auto root = Graph::build(); w = root->firstChild(); }
  EXPECT_TRUE(w.expired());
}
```
- For long-running services, add a soak test that runs N iterations and asserts RSS growth stays under a threshold.

## UBSan must be told to fail (verified)
By default UBSan **prints and continues**: a program with signed overflow exits 0 and CI stays green. Always add `-fno-sanitize-recover=all` (or `=undefined`) so the first UB aborts the run. `scripts/asan_check.sh` and the CMake option above already do.

## ThreadSanitizer job (separate build; cannot combine with ASan)
```bash
g++ -std=c++17 -g -O1 -fsanitize=thread -pthread main.cpp -o app_tsan && TSAN_OPTIONS=halt_on_error=1 ./app_tsan
```

## Hardening flags (GCC/libstdc++; each verified to be accepted or to trigger)
- `-D_GLIBCXX_ASSERTIONS` — makes `vector::operator[]`, `front()`, etc. abort on out-of-range (cheap; fine for release builds). Libc++ equivalent: `-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST` (not tested here).
- `-ftrivial-auto-var-init=zero` (GCC 12+/Clang 8+) — zero-initialises uninitialised locals; `=pattern` makes use of uninitialised values loud and deterministic in tests.
- `-Wconversion -Werror=narrowing` — GCC only *warns* on non-constant brace narrowing (`short s{d};`) unless `-Werror=narrowing` is set.
- `-fsanitize=float-cast-overflow` — flags out-of-range float→int casts (UB, not in the default UBSan set; Ariane 5's exact failure).
- `-fstack-protector-strong -D_FORTIFY_SOURCE=2` — cheap runtime mitigations for stack overflows (not tested here).
