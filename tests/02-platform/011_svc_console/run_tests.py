from __future__ import annotations
import re
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
RUNNER = ROOT / "vm" / "arm7-run" / "bin" / "arm7-run.exe"
KERNEL = HERE / "svc_kernel.bin"
USER = HERE / "svc_user.bin"

failures = 0

def check(ok, text):
    global failures
    print(f"  {'PASS' if ok else 'FAIL'} {text}")
    if not ok:
        failures += 1

def main():
    print("Running ARM7 bounded SVC console/kernel diagnostic...")

    check(RUNNER.is_file(), "arm7-run exists")
    check(KERNEL.is_file(), "kernel image exists")
    check(USER.is_file(), "user image exists")
    if not RUNNER.is_file() or not KERNEL.is_file() or not USER.is_file():
        return 1

    # CRT clear alone is ~10,000 instructions/cycles in this implementation.
    # Use a still-bounded budget large enough to reach all user SVC calls.
    script = "\n".join([
        f"load {KERNEL} 0x00000000",
        f"load {USER} 0x00010000",
        "set pc 0x00000000",
        "step 30000",
        "show crt",
        "e 0x0003f000-0x0003f00f",
        "regs",
        "quit",
        "",
    ])

    try:
        p = subprocess.run(
            [str(RUNNER)],
            input=script,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            cwd=ROOT,
            timeout=15,
            check=False,
        )
    except subprocess.TimeoutExpired as ex:
        print("  FAIL bounded step itself timed out")
        if ex.stdout:
            print(ex.stdout)
        return 1

    out = p.stdout
    check(p.returncode == 0, "arm7-run bounded automation completed")
    check("SVC puts works" in out, "first SVC puts completed")
    check("ABC" in out, "SVC putc sequence completed")
    check("kernel owns console" in out, "second SVC puts completed")
    check("[BKPT]" in out, "SVC exit reached kernel BKPT")
    # arm7-run prints 16-byte rows. RESULT_STATE is +4 inside the row
    # beginning at 0x0003F000, so accept the bytes in that row rather than
    # requiring a synthetic 0x0003F004 line.
    result_row = re.search(r"0x0003f000:\s+([^\\r\\n]+)", out, re.I)
    result_bytes = result_row.group(1).lower().split() if result_row else []
    check(len(result_bytes) >= 8 and result_bytes[4:8] == ["2a", "00", "00", "00"],
          "kernel recorded exit status 42")

    if failures:
        print("ARM7 bounded SVC console/kernel diagnostic: FAIL")
        print("\n--- bounded arm7-run snapshot ---")
        print(out)
        return 1

    print("ARM7 bounded SVC console/kernel diagnostic: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
