# Tooling Layers: What Each Tool Catches (and Misses)

All results below come from running the tools against this skill's BAD/GOOD cases (Ubuntu 24.04, GCC 13.3, Clang/LLVM from apt, cppcheck, Valgrind, CBMC 5.95.1). Re-run with `python3 scripts/tests/run_tools.py`.

## Contents
1. One-command check
2. Install
3. Coverage matrix and blind spots
4. Fuzzing
5. Bounded model checking with CBMC
6. Not bundled

## 1. One-command check
```bash
bash scripts/check_all.sh file.cpp [--msan] [-- extra flags]
```
Prints PASS/FAIL/SKIP per layer: `warnings`, `asan+ubsan`, `tsan` (only when threads are used), `cppcheck`, `clang-tidy`, `valgrind` (with fd-leak tracking), `msan` (opt-in). Missing tools are SKIPped. Exit 0 means no layer FAILED. Layers `asan`, `tsan`, `valgrind`, `msan` **execute the program**, and a non-zero program exit code counts as FAIL. Single translation unit only; for a real project use the CMake/CI setup in `build-and-ci.md`.

## 2. Install (Debian/Ubuntu)
```bash
sudo apt install cppcheck valgrind clang clang-tidy cbmc
```
`pip install cbmc` installs an **unrelated** package, not the verifier. Use the OS package.

## 3. Coverage matrix and blind spots (verified)
| Bug | ASan/UBSan | TSan | Valgrind | cppcheck | clang-tidy | MSan |
|---|---|---|---|---|---|---|
| Heap leak | ✔ | – | ✔ | ✔ | ✔ (simple paths) | – |
| Unclosed `fopen`/`open` handle | ✘ | – | ✔ (`--track-fds=yes`) | – | – | – |
| Use-after-free | ✔ | – | ✔ | ✔ simple cases | – | – |
| Out-of-bounds via variable index (`v[argc+3]`) | ✔ | – | ✔ | ✘ missed | ✘ | – |
| `strcpy` into small stack buffer | ✔ | – | ✔ | ✔ | ✔ | – |
| Dangling `string_view` | ✔ | – | ✔ | ✔ (`invalidLifetime`) | – | – |
| Returned lambda capturing a local by reference | ✔ (`detect_stack_use_after_return=1`) | – | ✘ reported 0 errors | – | – | – |
| Iterator invalidation in range-for | ✔ | – | – | ✔ (`invalidContainerLoop`) | – | – |
| Signed overflow | ✔ UBSan (no-recover) | – | ✘ | ✘ missed | – | – |
| Uninitialised local | ✘ | – | ✔ | – | – | ✔ |
| Uninitialised member | ✘ | – | ✔ | ✔ | ✔ (`pro-type-member-init`) | ✔ |
| Data race | ✘ | ✔ | ✘ | ✘ | ✘ | – |
Blind-spot rows marked ✘ for cppcheck/Valgrind are version-dependent; `run_tools.py` prints GAP-CONFIRMED or GAP-CLOSED so you notice when a newer tool fixes one.

Takeaway: no single tool is enough. ASan+UBSan is the best first layer; add TSan for threaded code, Valgrind when you need fd leaks or can't recompile, cppcheck/clang-tidy for cheap static checks, MSan for uninitialised reads.

## 4. Fuzzing (untrusted input)
Use when code parses or deserialises data from outside (files, network, user strings).
```bash
cp scripts/fuzz_template.cpp my_target.cpp      # edit: call your parser inside LLVMFuzzerTestOneInput
bash scripts/fuzz_run.sh my_target.cpp 60 corpus/
```
libFuzzer + ASan + UBSan (needs clang). A crash saves `crash-*`; reproduce with `./target crash-<hash>`. Keep and commit the corpus; run a short fuzz pass in CI and longer ones nightly. A clean run is **not** a proof, only "nothing found in N seconds". Tested: finds a planted stack overflow within seconds; a bounds-checked target stays clean.

## 5. Bounded model checking with CBMC (numeric ranges, small C kernels)
```bash
bash scripts/cbmc_check.sh formal/narrow_bad.c      # exit 10 + counterexample
bash scripts/cbmc_check.sh formal/narrow_good.c     # exit 0, VERIFICATION SUCCESSFUL
```
Pattern (the Ariane 5 question: "can this conversion ever overflow?"):
```c
int main(void){ double d; __CPROVER_assume(d > -1e6 && d < 1e6); (void)to_i16(d); return 0; }
```
- Unguarded `(int16_t)d` is refuted: `arithmetic overflow on float to signed integer type conversion`.
- A guarded version (`if (!(d >= -32768.0 && d < 32768.0)) return 0;`) is proved for the whole assumed input range.
- Results hold **within the assumptions and unwind bound** you give; loops need `--unwind N` (the script adds `--unwinding-assertions` so an insufficient bound fails instead of silently passing).
- C is well supported; C++ support is partial, so extract the numeric core into a C-compatible function.

## 6. Not bundled (and why)
CodeQL, SonarQube/SonarCloud, PVS-Studio, Coverity, Klocwork need accounts, licences or servers; GWP-ASan is a production-runtime feature; Dr. Memory, MSVC `/analyze`, Instruments are Windows/macOS-only and untested here; AFL++ was not installable via pip in the test environment (libFuzzer used instead). Link to them from your project docs if your organisation already has them.
