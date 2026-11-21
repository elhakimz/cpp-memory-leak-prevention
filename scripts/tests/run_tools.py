#!/usr/bin/env python3
"""Checks for the extra tooling: check_all.sh layers, fuzz_run.sh, cbmc_check.sh.
Tools that are not installed are SKIPped, not failed. 'GAP' lines document a tool's blind spot
(tool-version dependent): they print GAP-CONFIRMED / GAP-CLOSED and never fail the run."""
import subprocess, shutil, pathlib, re, sys, tempfile, os
here = pathlib.Path(__file__).parent; scripts = here.parent; src = here / "src"
have = shutil.which
bad = 0
def result(name, ok, extra=""):
    global bad; print(f"{name:44} {'PASS' if ok else 'FAIL'} {extra}"); bad += (not ok)

def layers(case, extra_args=()):
    r = subprocess.run(["bash", str(scripts / "check_all.sh"), str(src / f"{case}.cpp"), *extra_args], capture_output=True, text=True)
    st = {m.group(1): m.group(2) for m in re.finditer(r"^(\S+)\s+(PASS|FAIL|SKIP)", r.stdout, re.M)}
    return r.returncode, st

# (case, extra args, {layer: expected}, required tools)
CHECKS = [
 ("c01_good", (), {"warnings":"PASS","asan+ubsan":"PASS","clang-tidy":"PASS","cppcheck":"PASS","valgrind":"PASS"}, []),
 ("c05_bad", (), {"asan+ubsan":"FAIL","valgrind":"FAIL"}, ["valgrind"]),
 ("c04_fd", (), {"asan+ubsan":"PASS","valgrind":"FAIL"}, ["valgrind"]),          # LSan misses fd leaks, Valgrind --track-fds catches
 ("d03_stackbuf_bad", (), {"asan+ubsan":"FAIL","cppcheck":"FAIL"}, ["cppcheck"]),
 ("d09_member_bad", (), {"asan+ubsan":"PASS","clang-tidy":"FAIL","valgrind":"FAIL"}, ["clang-tidy","valgrind"]),
 ("d12_race_bad", (), {"tsan":"FAIL","asan+ubsan":"PASS","valgrind":"PASS"}, ["valgrind"]),  # only TSan sees the race
 ("d09_uninit_bad", ("--msan",), {"msan":"FAIL","valgrind":"FAIL"}, ["clang++","valgrind"]),
 ("d02_oob_good", ("--msan",), {"msan":"PASS"}, ["clang++"]),                     # no STL false positive seen in this small case
 ("d06_iter_good", ("--msan",), {"msan":"PASS"}, ["clang++"]),
]
for case, args, exp, need in CHECKS:
    missing = [t for t in need if not have(t)]
    if missing: print(f"{'check_all '+case:44} SKIP (missing {', '.join(missing)})"); continue
    rc, st = layers(case, args)
    ok = all(st.get(k) == v for k, v in exp.items())
    ok = ok and (rc == 0) == all(v != "FAIL" for v in st.values())
    result(f"check_all {case}", ok, "" if ok else f"got {st} rc={rc}")

# ---- documented blind spots (informational) ----
def gap(name, confirmed):
    print(f"{name:44} {'GAP-CONFIRMED' if confirmed else 'GAP-CLOSED (update references/tooling.md)'}")
if have("cppcheck"):
    for case in ("d02_oob_bad", "d07_ovf_bad"):
        r = subprocess.run(["cppcheck", "--enable=warning,portability", "--error-exitcode=1", "-q", str(src/f"{case}.cpp")], capture_output=True, text=True)
        gap(f"cppcheck misses {case}", r.returncode == 0)
if have("valgrind"):
    exe = tempfile.mktemp(); subprocess.run(["g++", "-g", "-O0", "-std=c++17", str(src/"d05_lambda_bad.cpp"), "-o", exe], check=True)
    r = subprocess.run(["valgrind", "--error-exitcode=9", exe], capture_output=True, text=True)
    gap("valgrind misses d05_lambda_bad (stack UAR)", "ERROR SUMMARY: 0 errors" in r.stderr); os.unlink(exe)

# ---- fuzzing ----
if have("clang++"):
    with tempfile.TemporaryDirectory() as d:
        r = subprocess.run(["bash", str(scripts/"fuzz_run.sh"), str(src/"fuzz"/"fz_bad.cpp"), "15"], capture_output=True, text=True, cwd=d)
        result("fuzz_run fz_bad finds overflow", r.returncode != 0 and "AddressSanitizer: stack-buffer-overflow" in r.stdout + r.stderr)
        for tgt in (src/"fuzz"/"fz_good.cpp", scripts/"fuzz_template.cpp"):
            r = subprocess.run(["bash", str(scripts/"fuzz_run.sh"), str(tgt), "5"], capture_output=True, text=True, cwd=d)
            result(f"fuzz_run {tgt.name} stays clean", r.returncode == 0)
else: print(f"{'fuzz_run':44} SKIP (clang++ missing)")

# ---- CBMC ----
if have("cbmc"):
    r = subprocess.run(["bash", str(scripts/"cbmc_check.sh"), str(src/"formal"/"narrow_bad.c")], capture_output=True, text=True)
    result("cbmc narrow_bad is refuted", r.returncode == 10 and "float to signed integer type conversion" in r.stdout and "VERIFICATION FAILED" in r.stdout)
    r = subprocess.run(["bash", str(scripts/"cbmc_check.sh"), str(src/"formal"/"narrow_good.c")], capture_output=True, text=True)
    result("cbmc narrow_good is proved", r.returncode == 0 and "VERIFICATION SUCCESSFUL" in r.stdout)
else: print(f"{'cbmc':44} SKIP (apt install cbmc)")
print("ALL PASS" if not bad else f"{bad} FAILED"); sys.exit(1 if bad else 0)
