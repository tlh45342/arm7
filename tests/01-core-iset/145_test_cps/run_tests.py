import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_cps"

CHECKS = [
    # snapshots of CPSR before/after each CPS
    ("MRS snap0",            "00008008:       E10F0000"),
    ("Store snap0",          "0000800C:       E5860000"),

    ("CPSID if",             "00008010:       F10C00C0"),
    ("MRS snap1",            "00008014:       E10F1000"),
    ("Store snap1",          "00008018:       E5861004"),

    ("CPSIE i",              "0000801C:       F1080080"),
    ("MRS snap2",            "00008020:       E10F2000"),
    ("Store snap2",          "00008024:       E5862008"),

    ("CPSID a",              "00008028:       F10C0100"),
    ("MRS snap3",            "0000802C:       E10F3000"),
    ("Store snap3",          "00008030:       E586300C"),

    ("CPSIE i, #0x13",       "00008034:       F10A0093"),
    ("MRS snap4",            "00008038:       E10F4000"),
    ("Store snap4",          "0000803C:       E5864010"),

    # trap
    ("BKPT",                 "00008040:       E1212374"),
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