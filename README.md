# cpp-memory-leak-prevention

A Claude skill for preventing, detecting, and fixing memory and resource-safety bugs in C++ projects. It started as a leak-prevention skill and now also covers lifetime bugs, undefined behavior, narrowing conversions, and data races.

Every code pattern in the skill is backed by a runnable BAD/GOOD test, so its advice is checked against real compiler and sanitizer output.

## What it covers

| Defect | Detector used in the tests | Reference |
|---|---|---|
| Memory and resource leaks | AddressSanitizer + LeakSanitizer | `references/patterns.md` |
| Use-after-free, dangling `string_view` / lambda captures, iterator invalidation | ASan | `references/memory-safety-errors.md` |
| Out-of-bounds access | ASan, `-D_GLIBCXX_ASSERTIONS` | `references/memory-safety-errors.md` |
| Signed overflow, null dereference | UBSan (`-fno-sanitize-recover=all`) | `references/memory-safety-errors.md` |
| Uninitialised data | `-ftrivial-auto-var-init=pattern`, clang-tidy | `references/memory-safety-errors.md` |
| Narrowing and float-to-int conversions (the Ariane 5 failure class) | `-Wconversion`, `-Werror=narrowing`, `-fsanitize=float-cast-overflow` | `references/memory-safety-errors.md` |
| Object slicing | Test, clang-tidy | `references/memory-safety-errors.md` |
| Data races | ThreadSanitizer | `references/memory-safety-errors.md` |

The core of the skill is a set of thirteen ownership rules (single owner expressed in the type, no naked `new`/`delete`, RAII for C handles, Rule of Zero/Five, `weak_ptr` to break cycles, and so on), a review checklist, and a legacy-to-modern refactoring path. It also covers logical leaks that sanitizers cannot see, such as unbounded caches and listeners that are never removed.

## When Claude uses it

The skill triggers when you work on C++ that involves ownership, smart pointers, RAII, raw pointers, `string_view`/`span` lifetimes, numeric casts, threads, or C APIs. It also triggers on terms such as memory leak, valgrind, AddressSanitizer, UBSan, ThreadSanitizer, segfault, dangling pointer, undefined behavior, and "make this C++ safe".

## Contents

```
cpp-memory-leak-prevention/
├── SKILL.md                         workflow, ownership rules, defect→detector table, review checklist
├── references/
│   ├── patterns.md                  10 leak patterns, before/after
│   ├── memory-safety-errors.md      non-leak defects, checked_narrow, Ariane 5 and contracts notes
│   ├── debugging-tools.md           ASan/LSan, Valgrind, heaptrack, MSVC CRT, macOS leaks; what LSan cannot see
│   └── build-and-ci.md              CMake options, clang-tidy config, CI job, hardening flags, TSan job
└── scripts/
    ├── asan_check.sh                build one file with ASan+UBSan+LSan and run it (exit 0 = clean)
    └── tests/
        ├── run_evals.py             18 leak-pattern checks
        ├── run_memsafety.py         31 checks for the other defect classes
        └── src/                     the BAD/GOOD C++ snippets
```

## Installing

Save `cpp-memory-leak-prevention.skill` through the **Save skill** button on the file card in Claude (available when your organization allows skill creation), or add it to your Claude skills directory.

## Running the tests

Requirements: `g++` with sanitizer support (tested with GCC 13.3), Python 3, and `clang-tidy` for the two member-initialisation cases.

```bash
python3 scripts/tests/run_evals.py          # leak patterns
python3 scripts/tests/run_memsafety.py      # everything else
bash scripts/asan_check.sh path/to/file.cpp # check your own file
```

Each BAD snippet must be caught (by a sanitizer, warning, or failed assertion) and each GOOD snippet must run clean. A runner ends with `ALL PASS` or the number of failures.

## Things the tests showed

- UBSan prints an error but exits 0 unless built with `-fno-sanitize-recover=all`, so CI stays green on undefined behavior by default.
- LeakSanitizer does not report unclosed `fopen` handles, and does not report a `shared_ptr` cycle that a `weak_ptr` still points into.
- GCC 13 did not warn on `int x; if (c) x = 1; return x;` at `-O2`.
- GCC only warns on non-constant brace narrowing (`short s{d};`) unless `-Werror=narrowing` is set.
- `std::unique_ptr<FILE, decltype(&fclose)>` triggers `-Wignored-attributes` on GCC; use an explicit deleter struct.
- A defaulted move assignment on an RAII subscription type leaks the old subscription.

## Not verified here

These parts of the skill come from general knowledge, not from running them in the test environment: Valgrind, heaptrack, MemorySanitizer, MSVC CRT debug heap, macOS Instruments and `leaks`, libc++ hardening, `_FORTIFY_SOURCE`, the GitHub Actions YAML, and C++26 contracts (standardised in 2026; the test machine has GCC 13, and the skill tells you to check your compiler's documentation for flags).

## Scope notes

- Tests are Linux/GCC based. Sanitizer support differs on macOS and Windows.
- `.skill` packaging excludes a root-level `evals/` folder, which is why the tests live under `scripts/tests/`.
