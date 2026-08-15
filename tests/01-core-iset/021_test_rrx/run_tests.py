import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_rrx"

CHECKS = [
    ("Loaded image", "[LOAD] test_rrx.bin @ 0x00008000"),
    ("PC start", "r15 <= 0x00008000"),

    ("Case1 result C=1,value=2", "[STR pre-imm] [0x00100000] <= r4 (0x80000001)"),
    ("Case1 CPSR",              "[STR pre-imm] [0x00100004] <= r10 (0x80000000)"),

    ("Case2 result C=0,value=2", "[STR pre-imm] [0x00100008] <= r7 (0x00000001)"),
    ("Case2 CPSR",              "[STR pre-imm] [0x0010000C] <= r10 (0x00000000)"),

    ("Case3 result C=0,value=1", "[STR pre-imm] [0x00100010] <= r8 (0x00000000)"),
    ("Case3 CPSR",              "[STR pre-imm] [0x00100014] <= r10 (0x60000000)"),

    ("Case4 result C=1,value=1", "[STR pre-imm] [0x00100018] <= r9 (0x80000000)"),
    ("Case4 CPSR",              "[STR pre-imm] [0x0010001C] <= r10 (0xA0000000)"),

    ("Plain RRX result",         "[STR pre-imm] [0x00100020] <= r1 (0x80000001)"),
    ("CPSR before plain RRX",    "[STR pre-imm] [0x00100024] <= r12 (0x60000000)"),
    ("CPSR after plain RRX",     "[STR pre-imm] [0x00100028] <= r13 (0x60000000)"),

    ("BKPT", "[BKPT]"),
    ("Final r1", "r1  = 0x80000001"),
    ("Final r4", "r4  = 0x80000001"),
    ("Final r7", "r7  = 0x00000000"),
    ("Final r9", "r9  = 0x80000000"),
    ("Final PC", "r15 = 0x000080A0"),
    ("Final CPSR", "CPSR = 0x60000000"),
    ("Cycle count", "cycle=41"),
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