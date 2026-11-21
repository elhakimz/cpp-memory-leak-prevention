#!/usr/bin/env bash
# Layered safety check for ONE C++ translation unit.
# Usage: check_all.sh file.cpp [--msan] [-- extra g++ flags]
# Layers (each prints PASS / FAIL / SKIP): warnings, asan+ubsan, tsan (only if threads used),
#   cppcheck, clang-tidy, valgrind (incl. fd leaks), msan (opt-in: --msan, clang only).
# NOTE: asan/tsan/valgrind/msan EXECUTE the program. Exit code 0 = no layer FAILED (SKIP is not failure).
set -u
src="${1:?usage: check_all.sh file.cpp [--msan] [-- extra flags]}"; shift
msan=0; extra=()
while [ $# -gt 0 ]; do
  case "$1" in --msan) msan=1 ;; --) shift; extra=("$@"); break ;; *) extra+=("$1") ;; esac; shift
done
CXX="${CXX:-g++}"; STD="${STD:-c++17}"
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
fail=0
have() { command -v "$1" >/dev/null 2>&1; }
report() { printf '%-14s %s%s\n' "$1" "$2" "${3:+  ($3)}"; [ "$2" = FAIL ] && fail=1; return 0; }
layer() { # name fn
  local name="$1" fn="$2" out
  if out="$("$fn" 2>&1)"; then report "$name" PASS
  else report "$name" FAIL; printf '%s\n' "$out" | head -n "${SHOW:-8}" | sed 's/^/    /'; fi
}
X=(${extra[@]+"${extra[@]}"})
l_warn()  { $CXX -std=$STD -fsyntax-only ${WARN_FLAGS:--Wall -Wextra -Wconversion -Wshadow -Wnon-virtual-dtor -Werror} "${X[@]}" "$src"; }
l_asan()  { $CXX -std=$STD -g -O0 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all "${X[@]}" "$src" -o "$tmp/asan" \
            && ASAN_OPTIONS=detect_leaks=1:detect_stack_use_after_return=1 "$tmp/asan"; }
l_tsan()  { $CXX -std=$STD -g -O1 -fsanitize=thread -pthread "${X[@]}" "$src" -o "$tmp/tsan" && TSAN_OPTIONS=halt_on_error=1 "$tmp/tsan"; }
l_cpp()   { cppcheck --enable=warning,portability --std=c++17 --error-exitcode=1 -q "$src"; }
l_tidy()  { clang-tidy "$src" -warnings-as-errors='*' \
            -checks='-*,clang-analyzer-*,bugprone-use-after-move,bugprone-unique-ptr-array-mismatch,cppcoreguidelines-pro-type-member-init,cppcoreguidelines-slicing,cppcoreguidelines-virtual-class-destructor' \
            -- -std=$STD "${X[@]}" 2>&1 | grep -E 'warning:|error:' && return 1; return 0; }
l_vg()    { $CXX -std=$STD -g -O0 -fno-omit-frame-pointer "${X[@]}" "$src" -o "$tmp/vg" || return 1
            valgrind --leak-check=full --track-origins=yes --track-fds=yes --error-exitcode=9 "$tmp/vg" >"$tmp/vg.out" 2>&1; local rc=$?
            grep -E 'Invalid (read|write)|uninitialised|definitely lost|indirectly lost|Open file descriptor [3-9]' "$tmp/vg.out" | head -n 5
            [ $rc -ne 0 ] && return 1; ! grep -qE 'Open file descriptor [3-9]' "$tmp/vg.out"; }
l_msan()  { clang++ -std=$STD -g -O1 -fno-omit-frame-pointer -fsanitize=memory -fsanitize-memory-track-origins "${X[@]}" "$src" -o "$tmp/msan" && "$tmp/msan"; }

layer warnings l_warn
layer asan+ubsan l_asan
if grep -qE '<thread>|<future>|<pthread.h>|std::async' "$src"; then layer tsan l_tsan; else report tsan SKIP "no threads used"; fi
if have cppcheck;   then layer cppcheck l_cpp;   else report cppcheck SKIP "not installed: apt install cppcheck"; fi
if have clang-tidy; then layer clang-tidy l_tidy; else report clang-tidy SKIP "not installed: apt install clang-tidy"; fi
if have valgrind;   then layer valgrind l_vg;    else report valgrind SKIP "not installed: apt install valgrind"; fi
if [ "$msan" = 1 ]; then
  if have clang++; then layer msan l_msan; else report msan SKIP "clang++ not installed"; fi
else report msan SKIP "opt-in (--msan); clang only; libstdc++ is not MSan-instrumented so STL-heavy code may give false positives"; fi
[ "$fail" = 0 ] && echo "RESULT: no layer failed" || echo "RESULT: at least one layer FAILED"
exit "$fail"
