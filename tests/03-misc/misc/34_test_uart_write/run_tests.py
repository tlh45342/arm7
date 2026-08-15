import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_uart_write"

CHECKS = [
    # arithmetic

    # sentinel / halt
    ("[HALT] DEADBEEF", "[HALT] DEADBEEF"),

    # final regs snapshot (exact spacing/case)
    ("regs r2", "r2  = 0x00000010"),
    ("regs r3", "r3  = 0x00000000"),
    ("regs r4", "r4  = 0x00000000"),
    ("regs r5", "r5  = 0x00000000"),
    ("regs r6", "r6  = 0x00000000"),
    ("cycle=476", "cycle=476")
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