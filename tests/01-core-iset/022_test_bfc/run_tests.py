import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_bfc"

CHECKS = [
    # setup
    ("Loaded image",          "[LOAD] test_bfc.bin @ 0x00008000"),
    ("PC start",              "r15 <= 0x00008000"),
    ("Base set (MOVT r6)",    "00008004:       E3406010"),

    # r0 case (clear all bits)
    ("BFC r0 opcode",         "00008010:       E7DF001F"),
    ("STR r0 -> [r6,#0]",     "00008014:       E5860000"),

    # r1 case (clear byte 8..15)
    ("BFC r1 opcode",         "00008020:       E7CF141F"),
    ("STR r1 -> [r6,#4]",     "00008024:       E5861004"),

    # r2 case (clear nibble 4..7)
    ("BFC r2 opcode",         "00008030:       E7C7221F"),
    ("STR r2 -> [r6,#8]",     "00008034:       E5862008"),

    # r3 case (clear bits 20..27)
    ("BFC r3 opcode",         "00008040:       E7DB3A1F"),
    ("STR r3 -> [r6,#12]",    "00008044:       E586300C"),

    # graceful stop
    ("BKPT",                  "00008048:       E1212374"),

    # final state (optional)
    ("Final PC",              "r15 = 0x00008048"),
    ("Final CPSR",            "CPSR = "),
    ("Cycle count",           "cycle="),
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