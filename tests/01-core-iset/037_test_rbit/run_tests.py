import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_rbit"

CHECKS = [
    ("Loaded image", "[LOAD] test_rbit.bin @ 0x00008000"),
    ("PC start", "r15 <= 0x00008000"),
    ("RBIT decoded", "[K12] RBIT match (key=0x6F3)"),

    ("Case A original", "[STR pre-imm] [0x00100000] <= r0 (0x00000001)"),
    ("Case A result",   "[STR pre-imm] [0x00100004] <= r1 (0x80000000)"),
    ("Case A CPSR",     "[STR pre-imm] [0x00100008] <= r7 (0x00000000)"),

    ("Case B original", "[STR pre-imm] [0x00100010] <= r0 (0x80000000)"),
    ("Case B result",   "[STR pre-imm] [0x00100014] <= r1 (0x00000001)"),
    ("Case B CPSR",     "[STR pre-imm] [0x00100018] <= r7 (0x00000000)"),

    ("Case C original", "[STR pre-imm] [0x00100020] <= r0 (0xF0F0F0F0)"),
    ("Case C result",   "[STR pre-imm] [0x00100024] <= r1 (0x0F0F0F0F)"),
    ("Case C CPSR",     "[STR pre-imm] [0x00100028] <= r7 (0x00000000)"),

    ("Case D original", "[STR pre-imm] [0x00100030] <= r0 (0x01234567)"),
    ("Case D result",   "[STR pre-imm] [0x00100034] <= r1 (0xE6A2C480)"),
    ("Case D CPSR",     "[STR pre-imm] [0x00100038] <= r7 (0x00000000)"),

    ("BKPT", "[BKPT]"),
    ("Final PC", "r15 = 0x0000808C"),
    ("Final CPSR", "CPSR = 0x00000000"),
    ("Cycle count", "cycle=36"),
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