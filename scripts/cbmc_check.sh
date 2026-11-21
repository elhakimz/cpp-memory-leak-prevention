#!/usr/bin/env bash
# Usage: cbmc_check.sh file.c [unwind=10] [extra cbmc flags]
# Bounded model checking with CBMC (install with the OS package manager: apt install cbmc).
# WARNING: `pip install cbmc` installs an UNRELATED package, not the verifier.
# Checks bounds, pointers, signed overflow, conversions (incl. float->int), unwinding. Use __CPROVER_assume / __CPROVER_assert in the harness.
# Exit 0 = VERIFICATION SUCCESSFUL (within the unwind bound), 10 = a property fails (counterexample printed).
set -u
src="${1:?usage: cbmc_check.sh file.c [unwind] [flags]}"; unwind="${2:-10}"; shift; shift 2>/dev/null || true
command -v cbmc >/dev/null || { echo "cbmc not installed: apt install cbmc"; exit 2; }
exec cbmc "$src" --bounds-check --pointer-check --signed-overflow-check --conversion-check --float-overflow-check \
  --div-by-zero-check --unwind "$unwind" --unwinding-assertions "$@"
