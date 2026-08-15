import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_bxx"

CHECKS = [
    # one-time base context (r6 high half)
    ("Base set (MOVT r6,#0x0100)", "00008004:       E3406100"),

    # key branch outcomes -> evidence = the STR landing at each slot
    ("EQ taken  -> [r6,#0]",      "00008024:       E5861000"),
    ("MI taken  -> [r6,#16]",     "000080A4:       E5861010"),
    ("VS not-taken -> [r6,#24]",  "000080E4:       E5861018"),  # fallthrough path
    ("VC taken  -> [r6,#28]",     "00008110:       E586101C"),
    ("HI taken  -> [r6,#32]",     "00008134:       E5861020"),  # unsigned >
    ("LT taken  -> [r6,#36]",     "00008158:       E5861024"),  # signed <
    ("LS taken  -> [r6,#52]",     "000081E8:       E5861034"),  # unsigned <=

    # graceful stop + final state
    ("BKPT",                      "000081EC:       E1212374"),
    ("Final PC",                  "r15 = 0x000081EC"),
    ("Final CPSR",                "CPSR = 0x80000000"),
    ("Cycle count",               "cycle=83"),
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