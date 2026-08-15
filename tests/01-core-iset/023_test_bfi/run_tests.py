import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_bfi"

CHECKS = [
    ("Loaded image", "[LOAD] test_bfi.bin @ 0x00008000"),
    ("PC start", "r15 <= 0x00008000"),

    ("Base MOVW", "00008000:       E3006000"),
    ("Base MOVT", "00008004:       E3406010"),

    ("BFI low field", "[STR pre-imm] [0x00100000] <= r0 (0x1234567A)"),
    ("BFI middle byte", "[STR pre-imm] [0x00100004] <= r2 (0xDEAD12EF)"),
    ("BFI 12-bit middle", "[STR pre-imm] [0x00100008] <= r4 (0xFFBDF000)"),
    ("BFI source masking", "[STR pre-imm] [0x0010000C] <= r7 (0xAAAA34AA)"),
    ("BFI high nibble", "[STR pre-imm] [0x00100010] <= r9 (0xC1234567)"),

    ("CPSR before BFI", "[STR pre-imm] [0x00100014] <= r11 (0x60000000)"),
    ("BFI flags-case result", "[STR pre-imm] [0x00100018] <= r12 (0x000000F0)"),
    ("CPSR after BFI", "[STR pre-imm] [0x0010001C] <= r13 (0x60000000)"),

    ("BKPT", "[BKPT]"),
    ("Final r0", "r0  = 0x1234567A"),
    ("Final r2", "r2  = 0xDEAD12EF"),
    ("Final r4", "r4  = 0xFFBDF000"),
    ("Final r7", "r7  = 0xAAAA34AA"),
    ("Final r9", "r9  = 0xC1234567"),
    ("Final r12", "r12 = 0x000000F0"),
    ("Final PC", "r15 = 0x00008090"),
    ("Final CPSR", "CPSR = 0x60000000"),
    ("Cycle count", "cycle=37"),
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
    sys.exit(0 if run_test() else 1)