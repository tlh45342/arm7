import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_mov"

CHECKS = [
    ("MOV imm #1",      "00008000:       E3A00042"),
    ("MOV imm #2",      "00008004:       E3A02081"),
    ("NOP #1",          "00008008:       E320F000"),
    ("MOV reg #1",      "0000800C:       E1A01000"),
    ("MOV imm #3",      "00008010:       E3A02082"),
    ("NOP #2",          "00008014:       E320F000"),
    ("MOV reg #2",      "00008018:       E1A03080"),
    ("MOVS imm",        "00008028:       E3B04001"),
    ("MOV reg flags",   "0000804C:       E1B05F80"),
    ("LSL reg shift #1","00008084:       E1B08110"),
    ("LSL reg shift #2","0000809C:       E1B09110"),
    ("Final BKPT",      "000080A8:       E1200070"),
    ("Final PC",        "r15 = 0x000080A8"),
    ("Final CPSR",      "CPSR = 0x20000000"),
    ("Cycle count",     "cycle=43"),
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