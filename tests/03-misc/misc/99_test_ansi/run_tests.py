import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_ansi"

CHECKS = [
    ("booted at 0x8000",            "Set r15 = 0x00008000"),

    # Using PL011-ish layout: FR at +0x18, TXFF bit (0x20) polled
    ("polls UART FR",               "mem[0x09000018]"),
    ("tests TXFF bit5",             "& 0x00000020"),

    # Using the right base
    ("has UART base in r2",         "[MOV imm] r2 = 0x09000000"),

    # Writes the expected byte stream to DR @ +0x00
    ("writes 'H'",                  "mem[0x09000000] <= r1 (0x00000048)"),
    ("writes 'e'",                  "mem[0x09000000] <= r1 (0x00000065)"),
    ("writes ' ' (space)",          "mem[0x09000000] <= r1 (0x00000020)"),
    ("writes 'W'",                  "mem[0x09000000] <= r1 (0x00000057)"),

    # CRLF behavior: explicit CR before LF
    ("inserts CR before LF",        "mem[0x09000000] <= r12 (0x0000000D)"),
    ("writes LF",                   "mem[0x09000000] <= r1 (0x0000000A)"),

    # Clean termination
    ("halts on DEADBEEF",           "[HALT] DEADBEEF"),
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