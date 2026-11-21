#!/usr/bin/env bash
# Usage: asan_check.sh file.cpp [extra g++ flags]
# Builds with ASan+UBSan (LeakSanitizer included on Linux) and runs. Exit 0 = no memory errors/leaks detected.
set -euo pipefail
src="${1:?usage: asan_check.sh file.cpp [flags]}"; shift || true
out="$(mktemp -u /tmp/asan_check.XXXXXX)"
${CXX:-g++} -std=c++17 -g -O0 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all "$@" "$src" -o "$out"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1:strict_string_checks=1}" "$out"
rc=$?; rm -f "$out"; exit $rc
