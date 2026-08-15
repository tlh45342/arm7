import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_cmn"

CHECKS = [
    # CMN cases (presence + MRS snapshot points)
    ("CMN #1",                "00008030:       E1730004"),
    ("MRS #1",                "00008034:       E10F7000"),

    ("CMN #2",                "00008044:       E1730004"),
    ("MRS #2",                "00008048:       E10F7000"),

    ("CMN overflow",          "0000805C:       E1730004"),
    ("MRS overflow",          "00008060:       E10F7000"),

    ("CMN negatives",         "00008074:       E1730004"),
    ("MRS negatives",         "00008078:       E10F7000"),

    ("CMN shift (LSL)",       "00008088:       E1730084"),
    ("MRS shift",             "0000808C:       E10F7000"),

    ("CMN RRX (uses C)",      "000080A0:       E1730064"),
    ("MRS RRX",               "000080A4:       E10F7000"),

    # graceful stop
    ("BKPT",                  "000080AC:       E1212374"),

    # final machine state
    ("Final PC",              "r15 = 0x000080AC"),
    ("Final CPSR",            "CPSR = 0x80000000"),
    ("Cycle count",           "cycle=44"),
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