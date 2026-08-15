import os
import subprocess
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_b"

CHECKS = [
    ("Loaded image", "[LOAD] test_b.bin @ 0x00008000"),

    ("Forward B skipped failure path",
     "[STR pre-imm] [0x00100000] <= r0 (0x00000001)"),

    ("Backward BNE loop completed",
     "[STR pre-imm] [0x00100004] <= r1 (0x00000000)"),

    ("BEQ taken path",
     "[STR pre-imm] [0x00100008] <= r2 (0x00000002)"),

    ("BEQ not-taken path",
     "[STR pre-imm] [0x0010000C] <= r3 (0x00000003)"),

    ("BKPT", "[BKPT]"),
]

def run_test():
    print(f"Running {TEST_NAME}...")

    script_path = f"{TEST_NAME}.script"
    bin_path = f"{TEST_NAME}.bin"
    log_path = f"{TEST_NAME}.log"

    if not os.path.exists(script_path):
        print(f"❌ Missing script: {script_path}")
        return False

    if not os.path.exists(bin_path):
        print(f"❌ Missing binary: {bin_path}")
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

    if proc.returncode not in (0,):
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
