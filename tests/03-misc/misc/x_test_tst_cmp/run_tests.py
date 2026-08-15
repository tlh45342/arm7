import subprocess
import os
import sys

VM = "arm-vm.exe"

TEST_NAME = "test_tst_cmp"

CHECKS = [
    # instruction stream (labels include context; matches use addr+word only)
    ("MOV r0,#0xF0           @8000", "00008000:       E3A000F0"),
    ("MOVT r0,#0xF0F0        @8004", "00008004:       E34F00F0"),
    ("TST r0,#0xF            @8008", "00008008:       E310000F"),
    ("MRS CPSR -> r4         @800C", "0000800C:       E10F4000"),
    ("TEQ (r0,...)           @8010", "00008010:       E1300000"),
    ("MRS CPSR -> r5         @8014", "00008014:       E10F5000"),
    ("MOV r1,#1              @8018", "00008018:       E3A01001"),
    ("CMP r1,#1              @801C", "0000801C:       E3510001"),
    ("MRS CPSR -> r6         @8020", "00008020:       E10F6000"),
    ("CMP r1,#2              @8024", "00008024:       E3510002"),
    ("MRS CPSR -> r7         @8028", "00008028:       E10F7000"),
    ("DEADBEEF trap          @802C", "0000802C:       DEADBEEF"),

    # final machine state
    ("Final PC",                 "r15 = 0x0000802C"),
    ("Final CPSR",               "CPSR = 0x80000000"),
    ("Cycle count",              "cycle=12"),
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