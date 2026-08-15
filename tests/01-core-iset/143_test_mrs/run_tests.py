import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_mrs"

CHECKS = [
    ("Binary loaded", "[LOAD] test_mrs.bin @ 0x00008000"),
    ("PC set",        "r15 <= 0x00008000"),

    ("MRS decoded",   "[K12] MRS match (key=0x100)"),

    ("CMP flags Z+C",     "r1  = 0x60000000"),
    ("SUBS flags N",      "r2  = 0x80000000"),
    ("ADDS flags N+V",    "r3  = 0x90000000"),
    ("Carry flags Z+C",   "r4  = 0x60000000"),
    ("CMP LR flags Z+C",  "r5  = 0x60000000"),
    ("TST flags",         "r6  = 0x60000000"),
    ("TEQ flags",         "r7  = 0xA0000000"),

    ("MSR decoded",       "[K12] MSR reg->CPSR match (key=0x120)"),
    ("MRS after MSR",     "r9  = 0xA0000000"),

    ("SP DEADBEEF",       "r13 = 0xDEADBEEF"),
    ("BKPT",              "[BKPT]"),
    ("Final PC",          "r15 = 0x00008070"),
    ("Final CPSR",        "CPSR = 0xA0000000"),
    ("Cycle count",       "cycle=29"),
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