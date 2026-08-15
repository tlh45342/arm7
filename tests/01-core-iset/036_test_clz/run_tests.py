import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_clz"

CHECKS = [
    ("Loaded image", "[LOAD] test_clz.bin @ 0x00008000"),
    ("PC start", "r15 <= 0x00008000"),

    ("CLZ decoded", "[K12] CLZ match (key=0x161)"),

    ("CLZ 0 -> 32",          "[STR pre-imm] [0x00100000] <= r0 (0x00000020)"),
    ("CLZ 1 -> 31",          "[STR pre-imm] [0x00100004] <= r0 (0x0000001F)"),
    ("CLZ 0x80000000 -> 0",  "[STR pre-imm] [0x00100008] <= r0 (0x00000000)"),
    ("CLZ 0x00F00000 -> 8",  "[STR pre-imm] [0x0010000C] <= r0 (0x00000008)"),
    ("CLZ 0x00008000 -> 16", "[STR pre-imm] [0x00100010] <= r0 (0x00000010)"),
    ("CLZ 0x12345678 -> 3",  "[STR pre-imm] [0x00100014] <= r0 (0x00000003)"),

    ("CPSR unchanged",       "[STR pre-imm] [0x00100018] <= r10 (0x00000000)"),

    ("CLZ 0xFFFFFFFF -> 0",  "[STR pre-imm] [0x0010001C] <= r0 (0x00000000)"),

    ("BKPT", "[BKPT]"),
    ("Final PC", "r15 = 0x0000807C"),
    ("Final CPSR", "CPSR = 0x00000000"),
    ("Cycle count", "cycle=32"),
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