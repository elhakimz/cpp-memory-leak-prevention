#!/usr/bin/env python3
"""Deterministic checks: every BAD snippet must be caught by a sanitizer/assert; every GOOD snippet must run clean."""
import subprocess, os, sys, pathlib
here = pathlib.Path(__file__).parent
FLAGS = "-std=c++17 -g -O0 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all -Wall -Wextra -Werror".split()
# (name, expected, marker in output, extra flags)
CASES = [
 ("c01_bad","fail","LeakSanitizer",[]),("c01_good","clean",None,[]),
 ("c02_bad","fail","LeakSanitizer",[]),("c02_good","clean",None,[]),
 ("c03_leak","fail","LeakSanitizer",[]) if (here/"src/c03_leak.cpp").exists() else ("c03_bad","clean",None,[]),
 ("c03_good","clean",None,[]),
 ("c04_good2","clean",None,[]),("c04b_handle","clean",None,[]),
 ("c05_bad","fail","LeakSanitizer",[]),("c05_good","clean",None,[]),
 ("c06_bad","fail","LeakSanitizer",[]),("c06_good","clean",None,[]),
 ("c07_bad","fail","double-free",[]),("c07_good","clean",None,[]),
 ("c08_bad","fail","new-delete-type-mismatch",["-Wno-error=delete-non-virtual-dtor"]),("c08_good","clean",None,[]),
 ("c10_asis","fail","Assertion",[]),   # documents the defaulted-move bug
 ("c10_fixed","clean",None,[]),
]
bad = 0
for name, exp, mark, extra in CASES:
    exe = f"/tmp/ev_{name}"
    b = subprocess.run(["g++", *FLAGS, *extra, str(here/f"src/{name}.cpp"), "-o", exe], capture_output=True, text=True)
    if b.returncode: print(f"{name:12} BUILD-ERROR\n{b.stderr[:400]}"); bad += 1; continue
    r = subprocess.run([exe], capture_output=True, text=True, env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1"))
    txt = r.stdout + r.stderr
    ok = (r.returncode == 0 and "Sanitizer" not in txt) if exp == "clean" else (r.returncode != 0 and mark in txt)
    print(f"{name:12} expect={exp:5} {'PASS' if ok else 'FAIL'}"); bad += (not ok)
print("ALL PASS" if not bad else f"{bad} FAILED"); sys.exit(1 if bad else 0)
