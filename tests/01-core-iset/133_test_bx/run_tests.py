import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_bx"

CHECKS = [
    ("Debug enabled", "[DEBUG] debug_flags set to 0x000003FF"),
    ("Loaded image", "[LOAD] test_bx.bin @ 0x00008000"),
    ("PC start", "r15 <= 0x00008000"),

    ("BX decoded", "[K12] BX (reg) match (key=0x121)"),

    ("BX r0 target reached", "[STR pre-imm] [0x00100000] <= r1 (0x000000AA)"),
    ("BX lr returned",       "[STR pre-imm] [0x00100004] <= r2 (0x00000022)"),

    ("BXEQ not taken",       "[STR pre-imm] [0x00100008] <= r7 (0x00000033)"),
    ("BXEQ taken",           "[STR pre-imm] [0x0010000C] <= r8 (0x00000055)"),

    ("CPSR preserved after BX", "[STR pre-imm] [0x00100010] <= r10 (0x60000000)"),

    ("BKPT", "[BKPT]"),
    ("Final PC", "r15 = 0x000080A0"),
    ("Final CPSR", "CPSR = 0x60000000"),
]

def run_test():
    print(f"Running {TEST_NAME}.script...")

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