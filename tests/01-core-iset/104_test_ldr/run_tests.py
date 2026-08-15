import os
import subprocess
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_ldr"

CHECKS = [
    ("Loaded image", "[LOAD] test_ldr.bin @ 0x00008000"),

    ("Plain LDR result",
     "[STR pre-imm] [0x00100100] <= r1 (0x11223344)"),

    ("Immediate-offset LDR result",
     "[STR pre-imm] [0x00100104] <= r2 (0x55667788)"),

    ("Post-index LDR result",
     "[STR pre-imm] [0x00100108] <= r3 (0x11223344)"),

    ("Post-index writeback",
     "[STR pre-imm] [0x0010010C] <= r8 (0x00100008)"),

    ("Pre-index LDR result",
     "[STR pre-imm] [0x00100110] <= r4 (0xDDEEFF00)"),

    ("Pre-index writeback",
     "[STR pre-imm] [0x00100114] <= r9 (0x0010000C)"),

    ("Register-offset LDR result",
     "[STR pre-imm] [0x00100118] <= r5 (0xCAFEBEEF)"),

    ("BKPT", "[BKPT]"),
]

def run_test():
    print(f"Running {TEST_NAME}...")

    script_path = f"{TEST_NAME}.script"
    bin_path = f"{TEST_NAME}.bin"
    log_path = f"{TEST_NAME}.log"

    for path in (script_path, bin_path):
        if not os.path.exists(path):
            print(f"❌ Missing file: {path}")
            return False

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

    with open(log_path, "r", encoding="utf-8", errors="replace") as f:
        log = f.read()

    passed = True
    for label, expected in CHECKS:
        if expected not in log:
            print(f"  ❌ Check failed: {label}")
            print(f"     Missing: {expected}")
            passed = False
        else:
            print(f"  ✅ {label}")

    print(f"{TEST_NAME}: {'✅ passed' if passed else '❌ failed'}")
    return passed

if __name__ == "__main__":
    sys.exit(0 if run_test() else 1)
