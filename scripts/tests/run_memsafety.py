#!/usr/bin/env python3
"""Deterministic checks for non-leak defects. Kinds: clean | fail (runtime marker) | cfail (compile error marker)."""
import subprocess, os, sys, pathlib
here = pathlib.Path(__file__).parent
ASAN = "-fsanitize=address,undefined -fno-sanitize-recover=all".split()
TSAN = ["-fsanitize=thread", "-pthread"]
BASE = "-std=c++17 -g -O0 -fno-omit-frame-pointer".split()
# name, kind, marker, sanitizer set, extra flags
C = [
 ("d01_uaf_bad","fail","heap-use-after-free",ASAN,[]),            ("d01_uaf_good","clean",None,ASAN,[]),
 ("d02_oob_bad","fail","heap-buffer-overflow",ASAN,[]),           ("d02_oob_good","clean",None,ASAN,[]),
 ("d02_hardened_bad","fail","Assertion",[],["-D_GLIBCXX_ASSERTIONS"]),
 ("d03_stackbuf_bad","fail","stack-buffer-overflow",ASAN,[]),    ("d03_stackbuf_good","clean",None,ASAN,[]),
 ("d04_sv_bad","fail","heap-use-after-free",ASAN,[]),            ("d04_sv_good","clean",None,ASAN,[]),
 ("d05_lambda_bad","fail","stack-use-after-return",ASAN,[]),     ("d05_lambda_good","clean",None,ASAN,[]),
 ("d05b_scope_bad","fail","stack-use-after-scope",ASAN,[]),
 ("d06_iter_bad","fail","heap-use-after-free",ASAN,[]),          ("d06_iter_good","clean",None,ASAN,[]),
 ("d07_ovf_bad","fail","signed integer overflow",ASAN,[]),       ("d07_ovf_good","clean",None,ASAN,[]),
 ("d08_null_bad","fail","null pointer",ASAN,[]),                         ("d08_null_good","clean",None,ASAN,[]),
 ("d09_uninit_bad","fail","UNINIT_VALUE_OBSERVED",[],["-ftrivial-auto-var-init=pattern"]),
 ("d09_uninit_good","clean",None,[],["-ftrivial-auto-var-init=pattern","-Wall","-Wextra","-Werror"]),
 ("d09_definite_bad","cfail","uninitialized",[],["-Wall","-Werror"]),
 ("d09_member_bad","tidy","pro-type-member-init",[],[]),
 ("d09_member_good","tidy-clean",None,[],[]),
 ("d10_conv_bad","cfail","may change value",[],["-Wconversion","-Werror"]),
 ("d10_brace_bad","cfail","narrowing",[],["-Werror=narrowing"]),
 ("d10_cast_ub_bad","fail","outside the range of representable values",ASAN,["-fsanitize=float-cast-overflow"]),
 ("d10_narrow_good","clean",None,ASAN,["-fsanitize=float-cast-overflow","-Wall","-Wextra","-Werror"]),
 ("d11_slice_bad","fail","Assertion",[],[]),                     ("d11_slice_good","clean",None,[],[]),
 ("d12_race_bad","fail","data race",TSAN,[]),                    ("d12_race_good","clean",None,TSAN,[]),
]
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:detect_stack_use_after_return=1", TSAN_OPTIONS="halt_on_error=1")
bad = 0
for name, kind, mark, san, extra in C:
    if kind in ("tidy","tidy-clean"):
        t = subprocess.run(["clang-tidy", str(here/f"src/{name}.cpp"), "--checks=-*,cppcoreguidelines-pro-type-member-init", "--", "-std=c++17"], capture_output=True, text=True).stdout
        ok = (mark in t) if kind == "tidy" else ("warning:" not in t)
        print(f"{name:18} expect={kind:5}  {'PASS' if ok else 'FAIL'}"); bad += (not ok); continue
    exe = f"/tmp/ms_{name}"
    b = subprocess.run(["g++", *BASE, *san, *extra, str(here/f"src/{name}.cpp"), "-o", exe], capture_output=True, text=True)
    if kind == "cfail":
        ok = b.returncode != 0 and mark in b.stderr
        print(f"{name:18} expect=cfail {'PASS' if ok else 'FAIL'}" + ("" if ok else "\n   " + b.stderr[:300])); bad += (not ok); continue
    if b.returncode: print(f"{name:18} BUILD-ERROR\n{b.stderr[:500]}"); bad += 1; continue
    r = subprocess.run([exe], capture_output=True, text=True, env=env); t = r.stdout + r.stderr
    ok = (r.returncode == 0 and "Sanitizer" not in t and "runtime error" not in t) if kind == "clean" else (r.returncode != 0 and mark in t)
    print(f"{name:18} expect={kind:5}  {'PASS' if ok else 'FAIL'} rc={r.returncode}" + ("" if ok else "\n   " + t[:350].replace("\n","\n   "))); bad += (not ok)
print("ALL PASS" if not bad else f"{bad} FAILED"); sys.exit(1 if bad else 0)
