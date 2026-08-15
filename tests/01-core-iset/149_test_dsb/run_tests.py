import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_dsb"

CHECKS = [
    # setup
    ("Loaded image",         "[LOAD] test_dsb.bin @ 0x00008000"),
    ("PC start",             "r15 <= 0x00008000"),

    # code (addr + word only)
    ("@8000 MOVW word",      "00008000:       E3006000"),
    ("@8004 MOVT word",      "00008004:       E3406010"),
    ("@8008 MRS  word",      "00008008:       E10F0000"),
    ("@800C STR  word",      "0000800C:       E5860000"),

    ("@8010 DSB  word",      "00008010:       F57FF04F"),
    ("@8014 MRS  word",      "00008014:       E10F1000"),
    ("@8018 EOR  word",      "00008018:       E0202001"),
    ("@801C STR  word",      "0000801C:       E5862004"),

    ("@8020 DSB  word",      "00008020:       F57FF04E"),
    ("@8024 MRS  word",      "00008024:       E10F3000"),
    ("@8028 EOR  word",      "00008028:       E0204003"),
    ("@802C STR  word",      "0000802C:       E5864008"),

    ("@8030 DSB  word",      "00008030:       F57FF04B"),
    ("@8034 MRS  word",      "00008034:       E10F5000"),
    ("@8038 EOR  word",      "00008038:       E0207005"),
    ("@803C STR  word",      "0000803C:       E586700C"),

    # graceful stop (BKPT encoding only)
    ("@8040 BKPT word",      "00008040:       E1212374"),
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