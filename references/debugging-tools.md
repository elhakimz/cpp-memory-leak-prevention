# Debugging Leaks: Tools and Reports

## Contents
- AddressSanitizer + LeakSanitizer (first choice)
- Valgrind
- heaptrack / massif (growth over time)
- MSVC CRT debug heap
- macOS leaks / Instruments
- Reading a report
- Triage for "memory grows but no leak reported"

## AddressSanitizer + LeakSanitizer (Linux/macOS, GCC/Clang)
```bash
g++ -std=c++17 -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all main.cpp -o app
ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1:detect_stack_use_after_return=1 ./app
```
LeakSanitizer runs at exit. On macOS, LSan support depends on the toolchain (Apple Clang may lack it; use Homebrew LLVM or `leaks`).
Suppressions: `LSAN_OPTIONS=suppressions=lsan.supp` with lines like `leak:ThirdPartyLib`.

## Valgrind
```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./app
```
Slower (10-50x) but needs no recompile. Use `-g` for line numbers.

## Growth over time (no leak at exit)
- `heaptrack ./app` then `heaptrack_gui heaptrack.app.*.gz` (flamegraph of allocations, peak, leaked).
- `valgrind --tool=massif ./app` then `ms_print massif.out.*`.
- Log `mallinfo2()` / RSS periodically in long-running services.
- tcmalloc/jemalloc heap profilers for production-like builds.

## MSVC (Windows)
```cpp
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
int main() { _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); /*...*/ }
```
Also: `/fsanitize=address` in VS 2019 16.9+, and Visual Studio Diagnostic Tools -> Memory Usage snapshots.

## macOS
`leaks --atExit -- ./app`, or Instruments -> Leaks / Allocations.

## What LSan cannot see (verified)
- Unclosed file descriptors / `FILE*` (glibc keeps open streams reachable) → `valgrind --track-fds=yes`, `lsof -p <pid>`, `ls /proc/<pid>/fd`.
- Memory still referenced from a global, a `weak_ptr`, or the stack at exit (reachable ≠ freed) → use heaptrack or `expired()` assertions.

## Reading an ASan/LSan report
- "Direct leak" = no pointer to the block remains; "Indirect leak" = reachable only from a leaked block (fix the direct one first).
- The allocation stack shows *where it was created*, not who should have freed it: walk up to find the owner that lost it.
- Same stack repeated N times = leak in a loop; count x size shows the cost.
- "still reachable" (Valgrind) is usually a global/static, not a bug unless it grows.

## Triage: memory grows but no leak reported
Likely logical leak (unbounded container/cache/listener list), allocator fragmentation, or `shared_ptr` cycle reachable from a global. Use heaptrack to see which allocation site dominates, then inspect what holds it.
