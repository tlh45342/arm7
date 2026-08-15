import subprocess
import os
import sys

VM = "arm-vm.exe"

TEST_NAME = "test_barriers"

CHECKS = [
    # setup / config
    ("Debug enabled",        "[DEBUG] debug_flags set to 0x000003FF"),
    ("Loaded image",         "[LOAD] test_barriers.bin @ 0x00008000"),
    ("PC start",             "r15 <= 0x00008000"),

    # instruction decode lines
    ("DMB shown",            "00008000:       F57FF05F        dmb sy"),
    ("DSB shown",            "00008004:       F57FF04F        dsb sy"),
    ("ISB shown",            "00008008:       F57FF06F        isb sy"),

    # graceful stop
    ("DEADBEEF trap",        "0000800C:       DEADBEEF"),

    # final machine state
    ("Final PC",             "r15 = 0x0000800C"),
    ("Final CPSR",           "CPSR = 0x00000000"),
    ("Cycle count",          "cycle=4"),
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