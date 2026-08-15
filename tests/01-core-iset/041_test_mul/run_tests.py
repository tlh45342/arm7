import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_mul"

CHECKS = [
    ("Loaded image", "[LOAD] test_mul.bin @ 0x00008000"),
    ("PC start", "r15 <= 0x00008000"),

    ("MUL/MULS decoded", "[K12] MUL/MULS match (key=0x019)"),

    ("Case 1: 0 * 123 -> 0", "r0  = 0x00000000"),
    ("Case 1: Z set",         "r8  = 0x40000000"),

    ("Case 2: 3 * 7 -> 21",   "r1  = 0x00000015"),
    ("Case 2: N/Z clear",     "r9  = 0x00000000"),

    ("Case 3: -1 * 5 -> -5",  "r4  = 0xFFFFFFFB"),
    ("Case 3: N set",         "r10 = 0x80000000"),

    ("Case 4: multiply",       "r3  = 0x2468ACF0"),
    ("Case 4: N/Z clear",     "r11 = 0x00000000"),

    ("Case 5: wrap -> 0",      "r5  = 0x00000000"),
    ("Case 5: Z set",          "r12 = 0x40000000"),

    ("BKPT",                   "[BKPT]"),
    ("Final PC",               "r15 = 0x00008068"),
    ("Final CPSR",             "CPSR = 0x40000000"),
    ("Cycle count",            "cycle=27"),
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