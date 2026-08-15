import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_bkpt"

CHECKS = [
    # setup
    ("Loaded image", "[LOAD] test_bkpt.bin @ 0x00008000"),
    ("PC start",     "r15 <= 0x00008000"),

    # key steps (comment/instruction tokens)
    ("Base set (MOVT r6,#0x0010)", "00008004:       E3406010"),
    ("Set r1 = #0x11",             "00008008:       E3A01011"),
    ("Store r1 -> [r6,#0]",        "0000800C:       E5861000"),

    # stop condition
    ("BKPT",                        "00008010:       E1212374"),
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

    try:
        subprocess.run(
            [VM],
            stdin=open(script_path, "r"),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )
    except FileNotFoundError:
        print(f"❌ Error: '{VM}' not found in PATH.")
        return False

    if not os.path.exists(log_path):
        print(f"❌ Missing log file: {log_path}")
        return False

    with open(log_path, "r") as f:
        log = f.read()

    passed = True
    for label, expected in CHECKS:
        if expected not in log:
            print(f"  ❌ Check failed: {label}")
            print(f"     Missing: {expected}")
            passed = False
        else:
            print(f"  ✅ {label}")

    print(f"{TEST_NAME}: {'✅ passed' if passed else '❌ failed'}\n")
    return passed

if __name__ == "__main__":
    success = run_test()
    sys.exit(0 if success else 1)