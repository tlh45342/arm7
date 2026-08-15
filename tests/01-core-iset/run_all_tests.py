#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
STATUS_FILE = ROOT / "ISA-STATUS.txt"
LOG_DIR = ROOT / "inventory-logs"

def classify_test_dir(path: Path):
    makefile = next((path / n for n in ("Makefile", "makefile") if (path / n).exists()), None)
    run_tests = path / "run_tests.py"

    if makefile is None:
        return ("NO MAKEFILE", 2, "")
    if not run_tests.exists():
        return ("NO TEST", 2, "")

    # Critical Windows fix:
    # The individual tests print Unicode PASS/FAIL glyphs. When stdout is
    # redirected to a pipe, Python on Windows may select cp1252 and crash
    # before the test completes. Force child Python processes to UTF-8.
    env = os.environ.copy()
    env["PYTHONIOENCODING"] = "utf-8"
    env["PYTHONUTF8"] = "1"

    try:
        proc = subprocess.run(
            ["make", "test"],
            cwd=path,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            env=env,
        )
    except FileNotFoundError:
        return ("MAKE NOT FOUND", 2, "")

    output = proc.stdout or ""

    if proc.returncode == 0:
        return ("PASS", 0, output)

    lower = output.lower()
    if "no rule to make target" in lower:
        return ("BUILD FAIL", proc.returncode, output)
    if "error:" in lower or "undefined reference" in lower:
        return ("BUILD FAIL", proc.returncode, output)
    if "missing script:" in lower or "missing binary:" in lower:
        return ("NO TEST", proc.returncode, output)

    return ("FAIL", proc.returncode, output)

def short_failure(output: str):
    lines = [line.rstrip() for line in output.splitlines() if line.strip()]
    interesting = []
    for line in lines:
        low = line.lower()
        if ("❌" in line or "error" in low or "failed" in low or
            "traceback" in low or "no rule to make target" in low or
            "undefined reference" in low or "missing " in low):
            interesting.append(line)
    return " | ".join((interesting or lines[-3:])[:3])

def main():
    test_dirs = sorted(p for p in ROOT.iterdir()
                       if p.is_dir() and "_test_" in p.name)

    if not test_dirs:
        print(f"No *_test_* directories found under {ROOT}")
        return 1

    LOG_DIR.mkdir(exist_ok=True)
    rows = []
    totals = {}

    print(f"ARM7 ISA test inventory: {ROOT}")
    print("=" * 72)

    for path in test_dirs:
        status, rc, output = classify_test_dir(path)
        totals[status] = totals.get(status, 0) + 1

        log_path = LOG_DIR / f"{path.name}.log"
        if output:
            log_path.write_text(output, encoding="utf-8", errors="replace")

        detail = short_failure(output) if status != "PASS" else ""
        rows.append((path.name, status, rc, detail,
                     log_path.name if output else ""))

        print(f"{path.name:<24} {status}")

    print("=" * 72)
    print(f"Total: {len(rows)}")

    order = ["PASS", "FAIL", "BUILD FAIL", "NO TEST",
             "NO MAKEFILE", "MAKE NOT FOUND"]
    for key in order:
        if key in totals:
            print(f"{key:<12}: {totals[key]}")
    for key in sorted(k for k in totals if k not in order):
        print(f"{key:<12}: {totals[key]}")

    with STATUS_FILE.open("w", encoding="utf-8", newline="\n") as f:
        f.write("ARM7 ISA TEST STATUS\n")
        f.write("====================\n\n")
        f.write(f"Root: {ROOT}\n")
        f.write(f"Total test directories: {len(rows)}\n\n")
        f.write("SUMMARY\n-------\n")
        for key in order:
            if key in totals:
                f.write(f"{key:<12}: {totals[key]}\n")
        f.write("\nDETAIL\n------\n")
        for name, status, rc, detail, log_name in rows:
            f.write(f"{name:<24} {status}")
            if status != "PASS":
                f.write(f" (rc={rc})")
            f.write("\n")
            if log_name:
                f.write(f"    log: inventory-logs/{log_name}\n")
            if detail:
                f.write(f"    {detail}\n")

    print(f"\nWrote: {STATUS_FILE}")
    print(f"Logs : {LOG_DIR}")

    bad = totals.get("FAIL", 0) + totals.get("BUILD FAIL", 0)
    return 0 if bad == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
