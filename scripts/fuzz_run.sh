#!/usr/bin/env bash
# Usage: fuzz_run.sh target.cpp [seconds=20] [corpus_dir]
# target.cpp must define: extern "C" int LLVMFuzzerTestOneInput(const uint8_t*, size_t)  (see fuzz_template.cpp)
# Builds with clang++ -fsanitize=fuzzer,address,undefined, runs for N seconds. Exit 0 = no crash found in that time
# (NOT a proof of absence). Crash inputs are saved as crash-* files in the current directory.
set -u
src="${1:?usage: fuzz_run.sh target.cpp [seconds] [corpus]}"; secs="${2:-20}"; corpus="${3:-}"
command -v clang++ >/dev/null || { echo "clang++ required (apt install clang)"; exit 2; }
exe="$(mktemp -u /tmp/fuzz.XXXXXX)"
clang++ -std=c++17 -g -O1 -fno-omit-frame-pointer -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=undefined "$src" -o "$exe" || exit 2
if [ -n "$corpus" ]; then mkdir -p "$corpus"; "$exe" -max_total_time="$secs" "$corpus"; else "$exe" -max_total_time="$secs"; fi
rc=$?; rm -f "$exe"; exit $rc
