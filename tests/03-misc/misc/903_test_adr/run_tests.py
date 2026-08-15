import subprocess
import os
import sys

VM = "arm-vm.exe"

TEST_NAME = "test_adr"

CHECKS = [
    # setup
    ("Loaded image", "[LOAD] test_adr.bin @ 0x00008000"),
    ("PC start",     "r15 <= 0x00008000"),

    # base pointer (MOVT)
    ("Base set",     "00008004:       E3406010"),

    # ADR-style PC-relative ops (need the exact instruction line)
    ("ADR fwd",      "00008008:       E28F000C"),
    ("ADR back",     "0000800C:       E24F100C"),

    # prove results used (stores)
    ("Store r0",     "0000801C:       E5860000"),
    ("Store r1",     "00008020:       E5861004"),

    # stop
    ("BKPT",         "00008024:       E1212374"),

    # final state
    ("Final r0",     "r0  = 0x0000801C"),
    ("Final r1",     "r1  = 0x00008008"),
    ("Final PC",     "r15 = 0x00008024"),
    ("Final CPSR",   "CPSR = 0x00000000"),
    ("Cycle count",  "cycle=10"),
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