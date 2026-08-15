import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_sbcs"

CHECKS = [
    # Setup
    ("Debug enabled", "[DEBUG] debug_flags set to 0xFFFFFFFF"),
    ("PC init", "Set r15 = 0x00008000"),
    ("CPSR cleared", "[PSR write] CPSR <= 0x00000000"),

    # Case 1: SBC with C=0  => 0 - 0 - 1
    ("r0=0", "[MOV imm] r0 = 0x00000000"),
    ("r1=0", "[MOV imm] r1 = 0x00000000"),
    ("SBC case1 result", "=> 0xFFFFFFFF  (N=1 Z=0 C=0 V=0)"),
    ("MRS captured N only", "[MRS] r10 = CPSR (0x80000000)"),
    ("Mask confirms N", "[AND imm] r10 = r10 (0x80000000) & 0xF0000000 => 0x80000000"),

    # Prep for case 2: set C=1
    ("ORR builds C bit", "[ORR] r9 = r9|op2 => 0x20000000"),
    ("CPSR write C=1", "[PSR write] CPSR <= 0x20000000"),

    # Case 2: intended SBC with C=1  => 0x80000000 - 1 - 0
    ("r3=0x80000000", "[MOV imm] r3 = 0x80000000"),
    ("r4=1", "[MOV imm] r4 = 0x00000001"),
    ("SBC case2 result (current)", "=> 0x7FFFFFFE  (N=0 Z=0 C=1 V=1)"),
    ("MRS captured C|V", "[MRS] r11 = CPSR (0x30000000)"),
    ("Mask confirms C|V", "[AND imm] r11 = r11 (0x30000000) & 0xF0000000 => 0x30000000"),

    # Halt
    ("HALT", "[HALT] DEADBEEF.  Halting VM."),
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