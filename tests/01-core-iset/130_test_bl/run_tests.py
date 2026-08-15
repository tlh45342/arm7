import os
import re
import subprocess
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_bl"

def run_test():
    print(f"Running {TEST_NAME}...")

    script_path = f"{TEST_NAME}.script"
    bin_path = f"{TEST_NAME}.bin"
    lst_path = f"{TEST_NAME}.lst"
    log_path = f"{TEST_NAME}.log"

    for path in (script_path, bin_path, lst_path):
        if not os.path.exists(path):
            print(f"❌ Missing file: {path}")
            return False

    # Read the linked listing so expected LR is derived from the actual BL
    # placement instead of hard-coding an address that may move later.
    listing = open(lst_path, "r", encoding="utf-8", errors="replace").read()

    m = re.search(
        r"^\s*([0-9a-fA-F]+):\s+[0-9a-fA-F]+\s+bl\s+.*<subroutine>",
        listing,
        re.MULTILINE,
    )
    if not m:
        print("❌ Could not locate BL instruction in test_bl.lst")
        return False

    bl_addr = int(m.group(1), 16)
    expected_lr = bl_addr + 4

    env = os.environ.copy()
    env["PYTHONIOENCODING"] = "utf-8"
    env["PYTHONUTF8"] = "1"

    try:
        with open(script_path, "r", encoding="utf-8") as script:
            proc = subprocess.run(
                [VM],
                stdin=script,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding="utf-8",
                errors="replace",
                env=env,
            )
    except FileNotFoundError:
        print(f"❌ Error: '{VM}' not found in PATH.")
        return False

    if proc.returncode != 0:
        print(f"❌ {VM} returned rc={proc.returncode}")
        if proc.stdout:
            print(proc.stdout)
        return False

    if not os.path.exists(log_path):
        print(f"❌ Missing log file: {log_path}")
        return False

    log = open(log_path, "r", encoding="utf-8", errors="replace").read()

    checks = [
        ("Loaded image", "[LOAD] test_bl.bin @ 0x00008000"),

        ("Subroutine reached",
         "[STR pre-imm] [0x00100000] <= r0 (0x11111111)"),

        ("BL wrote correct LR",
         f"[STR pre-imm] [0x00100004] <= r14 (0x{expected_lr:08X})"),

        ("Returned to caller",
         "[STR pre-imm] [0x00100008] <= r1 (0x22222222)"),

        ("BKPT", "[BKPT]"),
    ]

    passed = True
    for label, expected in checks:
        if expected not in log:
            print(f"  ❌ Check failed: {label}")
            print(f"     Missing: {expected}")
            passed = False
        else:
            print(f"  ✅ {label}")

    print(f"  {'✅' if passed else '❌'} Expected LR = 0x{expected_lr:08X}")
    print(f"{TEST_NAME}: {'✅ passed' if passed else '❌ failed'}")
    return passed

if __name__ == "__main__":
    sys.exit(0 if run_test() else 1)
