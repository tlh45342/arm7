import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_adcs"

CHECKS = [
    ("Debug enabled",  r"\[DEBUG\] debug_flags set to 0xFFFFFFFF"),
    ("PC init",        r"Set r15 = 0x00008000"),
    ("CPSR cleared",   r"\[PSR write\] CPSR <= 0x00000000 .*-> 0x00000000"),
    ("r0=0",           r"\[MOV imm\] r0 = 0x00000000"),
    ("r1=0",           r"\[MOV imm\] r1 = 0x00000000"),
    ("SBC case1 result", r"\[SBC reg\].*=> 0xFFFFFFFF\s+\(N=1 Z=0 C=0 V=0\)"),
    ("MRS captured N only", r"\[MRS\] r10 = CPSR \(0x80000000\)"),
    ("Mask confirms N", r"\[AND imm\] r10 = r10.*=> 0x80000000"),
    ("ORR builds C bit", r"\[ORR\] r9 = r9\|op2 => 0x20000000"),
    ("CPSR write C=1", r"\[PSR write\] CPSR <= 0x20000000"),
    ("r3=0x80000000",  r"\[MOV imm\] r3 = 0x80000000"),
    ("r4=1",           r"\[MOV imm\] r4 = 0x00000001"),
    ("SBC case2 result", r"\[SBC reg\].*\(N=0 Z=0 C=1 V=1\)"),
    ("MRS captured C|V", r"\[MRS\] r11 = CPSR \(0x30000000\)"),
    ("Mask confirms C|V", r"\[AND imm\] r11 = r11.*=> 0x30000000"),
    ("HALT",           r"\[HALT\] DEADBEEF\.  Halting VM\."),
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