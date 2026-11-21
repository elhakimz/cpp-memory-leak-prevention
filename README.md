# cpp-memory-leak-prevention

A Claude skill for preventing, detecting, and fixing memory and resource-safety bugs in C++ projects. It started as a leak-prevention skill and now also covers lifetime bugs, undefined behavior, narrowing conversions, data races, and a layered set of analysis tools (sanitizers, Valgrind, cppcheck, clang-tidy, fuzzing, and bounded model checking).

Every code pattern and tool claim in the skill is backed by a runnable test, so its advice is checked against real compiler and tool output.

## What it covers

| Defect | Detectors exercised in the tests | Reference |
|---|---|---|
| Memory and resource leaks | ASan + LeakSanitizer, Valgrind | `references/patterns.md` |
| Unclosed file handles | Valgrind `--track-fds=yes` (LSan misses these) | `references/tooling.md` |
| Use-after-free, dangling `string_view` / lambda captures, iterator invalidation | ASan, cppcheck | `references/memory-safety-errors.md` |
| Out-of-bounds access | ASan, Valgrind, `-D_GLIBCXX_ASSERTIONS` | `references/memory-safety-errors.md` |
| Signed overflow, null dereference | UBSan (`-fno-sanitize-recover=all`) | `references/memory-safety-errors.md` |
| Uninitialised data | Valgrind, MSan, `-ftrivial-auto-var-init=pattern`, clang-tidy | `references/memory-safety-errors.md` |
| Narrowing and float-to-int conversions (the Ariane 5 failure class) | `-Wconversion`, `-fsanitize=float-cast-overflow`, CBMC | `references/memory-safety-errors.md`, `references/tooling.md` |
| Object slicing | Test, clang-tidy | `references/memory-safety-errors.md` |
| Data races | ThreadSanitizer | `references/memory-safety-errors.md` |
| Bugs triggered by untrusted input | libFuzzer + ASan + UBSan | `references/tooling.md` |

The core of the skill is thirteen ownership rules (single owner expressed in the type, no naked `new`/`delete`, RAII for C handles, Rule of Zero/Five, `weak_ptr` to break cycles, and so on), a review checklist, and a legacy-to-modern refactoring path. It also covers logical leaks that sanitizers cannot see, such as unbounded caches and listeners that are never removed.

## When Claude uses it

The skill triggers when you work on C++ involving ownership, smart pointers, RAII, raw pointers, `string_view`/`span` lifetimes, numeric casts, threads, or C APIs. It also triggers on terms such as memory leak, valgrind, AddressSanitizer, UBSan, ThreadSanitizer, segfault, dangling pointer, undefined behavior, fuzzing, and "make this C++ safe".

## Contents

```
cpp-memory-leak-prevention/
├── SKILL.md                         workflow, ownership rules, defect→detector table, review checklist
├── references/
│   ├── patterns.md                  10 leak patterns, before/after
│   ├── memory-safety-errors.md      non-leak defects, checked_narrow, Ariane 5 and contracts notes
│   ├── tooling.md                   tool coverage matrix with verified blind spots, fuzzing, CBMC
│   ├── debugging-tools.md           ASan/LSan, Valgrind, heaptrack, MSVC CRT, macOS leaks
│   └── build-and-ci.md              CMake options, clang-tidy config, CI job, hardening flags, TSan job
└── scripts/
    ├── asan_check.sh                build one file with ASan+UBSan+LSan and run it
    ├── check_all.sh                 layered check: warnings, ASan+UBSan, TSan, cppcheck, clang-tidy, Valgrind, MSan (opt-in)
    ├── fuzz_run.sh / fuzz_template.cpp   libFuzzer harness runner and template
    ├── cbmc_check.sh                bounded model checking wrapper
    └── tests/
        ├── run_evals.py             18 leak-pattern checks
        ├── run_memsafety.py         31 checks for the other defect classes
        ├── run_tools.py             17 checks of check_all layers, fuzzing, CBMC, and tool blind spots
        └── src/                     the BAD/GOOD C++ and C snippets
```

## Installing

Save `cpp-memory-leak-prevention.skill` through the **Save skill** button on the file card in Claude (available when your organization allows skill creation), or add it to your Claude skills directory.

## Running the tests

```bash
sudo apt install g++ cppcheck valgrind clang clang-tidy cbmc   # Debian/Ubuntu; missing tools are skipped, not failed
python3 scripts/tests/run_evals.py          # leak patterns
python3 scripts/tests/run_memsafety.py      # lifetime, UB, numeric, races
python3 scripts/tests/run_tools.py          # tool layers, fuzzing, CBMC
bash scripts/check_all.sh path/to/file.cpp  # check your own file
```

Each BAD snippet must be caught and each GOOD snippet must run clean. A runner ends with `ALL PASS` or the number of failures. Do not install CBMC with `pip`: the PyPI package named `cbmc` is an unrelated project.

## Things the tests showed

- UBSan prints an error but exits 0 unless built with `-fno-sanitize-recover=all`, so CI stays green on undefined behavior by default.
- LeakSanitizer does not report unclosed `fopen` handles, and does not report a `shared_ptr` cycle that a `weak_ptr` still points into. Valgrind with `--track-fds=yes` caught the file handle.
- Valgrind reported 0 errors for a returned lambda that captures a local by reference; ASan caught it.
- cppcheck missed an out-of-bounds read through a variable index and a signed overflow; ASan and UBSan caught both. The runner prints GAP-CONFIRMED or GAP-CLOSED so you notice when a newer version fixes these.
- GCC 13 did not warn on `int x; if (c) x = 1; return x;` at `-O2`.
- GCC only warns on non-constant brace narrowing (`short s{d};`) unless `-Werror=narrowing` is set.
- `std::unique_ptr<FILE, decltype(&fclose)>` triggers `-Wignored-attributes` on GCC; use an explicit deleter struct.
- A defaulted move assignment on an RAII subscription type leaks the old subscription.
- CBMC refuted an unguarded double-to-int16 conversion with a counterexample and proved the range-guarded version.

## Not verified here

These parts come from general knowledge, not from running them in the test environment: heaptrack, MSVC CRT debug heap, macOS Instruments and `leaks`, libc++ hardening, `_FORTIFY_SOURCE`, the GitHub Actions YAML, and C++26 contracts (standardised in 2026; the test machine has GCC 13, and the skill tells you to check your compiler's documentation for flags). MemorySanitizer was only run on small examples, and libstdc++ is not MSan-instrumented, so STL-heavy code may give false positives.

## Scope notes

- Tests are Linux/GCC/Clang based. Sanitizer support differs on macOS and Windows.
- `check_all.sh` handles one translation unit and executes the program; use CMake and CI for whole projects.
- A clean fuzz run is not a proof of absence, and CBMC results hold only within the assumptions and unwind bound you give.
- `.skill` packaging excludes a root-level `evals/` folder, which is why the tests live under `scripts/tests/`.
