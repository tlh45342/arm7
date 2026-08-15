import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_usat"

CHECKS = [
    ("Loaded image", "[LOAD] test_usat.bin @ 0x00008000"),
    ("PC start", "r15 <= 0x00008000"),

    ("Case1 negative -> 0", "[STR pre-imm] [0x00100000] <= r0 (0x00000000)"),
    ("Case1 Q set",        "[STR pre-imm] [0x00100004] <= r10 (0x08000000)"),

    ("Case2 100 -> 100",   "[STR pre-imm] [0x00100008] <= r2 (0x00000064)"),
    ("Case2 Q clear",      "[STR pre-imm] [0x0010000C] <= r10 (0x00000000)"),

    ("Case3 255 -> 255",   "[STR pre-imm] [0x00100010] <= r3 (0x000000FF)"),
    ("Case3 Q clear",      "[STR pre-imm] [0x00100014] <= r10 (0x00000000)"),

    ("Case4 300 -> 255",   "[STR pre-imm] [0x00100018] <= r4 (0x000000FF)"),
    ("Case4 Q set",        "[STR pre-imm] [0x0010001C] <= r10 (0x08000000)"),

    ("Case5 shift -> 255", "[STR pre-imm] [0x00100020] <= r5 (0x000000FF)"),
    ("Case5 Q set",        "[STR pre-imm] [0x00100024] <= r10 (0x08000000)"),

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

    try:
        subprocess.run(
            [VM],
            stdin=open(script_path, "r"),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
    except FileNotFoundError:
        print(f"❌ Error: '{VM}' not found in PATH.")
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

    print(f"{TEST_NAME}: {'✅ passed' if passed else '❌ failed'}\n")
    return passed

if __name__ == "__main__":
    sys.exit(0 if run_test() else 1)