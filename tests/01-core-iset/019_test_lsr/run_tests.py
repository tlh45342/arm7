import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_lsr"

CHECKS = [
    # ------------------------------------------------------------------
    # Harness / setup checks
    # ------------------------------------------------------------------
    ("Loaded image",           "[LOAD] test_lsr.bin @ 0x00008000"),
    ("PC start",               "r15 <= 0x00008000"),

    # Sanity check: base pointer for STRs (r6 = 0x00100000)
    ("Base set (MOVT r6)",     "00008004:       E3406010"),

    # ------------------------------------------------------------------
    # Case 1: r0 = 0x80000001 >> 1 -> r1 = 0x40000000
    # Expect: C = 1, Z = 0  => CPSR = 0x20000000
    # Stored at [0x00100000] (value) and [0x00100004] (CPSR)
    # ------------------------------------------------------------------
    ("Case1 value 0x40000000",
     "[0x00100000] <= r1 (0x40000000)"),
    ("Case1 flags 0x20000000",
     "[0x00100004] <= r7 (0x20000000)"),

    # ------------------------------------------------------------------
    # Case 2: r0 = 0x80000001 >> 31 -> r2 = 0x00000001
    # Expect: C = 0, Z = 0  => CPSR = 0x00000000
    # Stored at [0x00100008] (value) and [0x0010000C] (CPSR)
    # ------------------------------------------------------------------
    ("Case2 value 0x00000001",
     "[0x00100008] <= r2 (0x00000001)"),
    ("Case2 flags 0x00000000",
     "[0x0010000C] <= r7 (0x00000000)"),

    # ------------------------------------------------------------------
    # Case 3: r0 = 0xFFFFFFFF >> 1 -> r3 = 0x7FFFFFFF
    # Expect: C = 1, Z = 0  => CPSR = 0x20000000
    # Stored at [0x00100010] (value) and [0x00100014] (CPSR)
    # ------------------------------------------------------------------
    ("Case3 value 0x7FFFFFFF", 
     "[0x00100010] <= r3 (0x7FFFFFFF)"),
    ("Case3 flags 0x20000000",
     "[0x00100014] <= r7 (0x20000000)"),

    # ------------------------------------------------------------------
    # Case 4: r0 = 0x00000001 >> 1 -> r4 = 0x00000000
    # Expect: C = 1, Z = 1  => CPSR = 0x60000000
    # Stored at [0x00100018] (value) and [0x0010001C] (CPSR)
    # ------------------------------------------------------------------
    ("Case4 value 0x00000000",
     "[0x00100018] <= r4 (0x00000000)"),
    ("Case4 flags 0x60000000",
     "[0x0010001C] <= r7 (0x60000000)"),

    # ------------------------------------------------------------------
    # Case 5: r0 = 0x80000000 >> 32 (encoded as LSR #0) -> r5 = 0x00000000
    # Expect: C = 1, Z = 1  => CPSR = 0x60000000
    # Stored at [0x00100020] (value) and [0x00100024] (CPSR)
    # ------------------------------------------------------------------
    ("Case5 value 0x00000000",
     "[0x00100020] <= r5 (0x00000000)"),
    ("Case5 flags 0x60000000",
     "[0x00100024] <= r7 (0x60000000)"),

    # ------------------------------------------------------------------
    # Graceful stop: BKPT and final machine state
    # ------------------------------------------------------------------
    ("BKPT",                   "00008078:       E1212374"),
    ("Final PC",               "r15 = 0x00008078"),
    ("Final CPSR",             "CPSR = 0x60000000"),
    ("Cycle count",            "cycle=31"),
]


def run_test():
    print(f"Running {TEST_NAME}.script...")

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