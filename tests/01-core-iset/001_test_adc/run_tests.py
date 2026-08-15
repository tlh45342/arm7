import subprocess
import os
import sys

VM = "arm7-run.exe"

TEST_NAME = "test_adc"

CHECKS = [
    # setup
    ("Loaded image", "[LOAD] test_adc.bin @ 0x00008000"),
    ("PC start",     "r15 <= 0x00008000"),

    # base pointer for stores
    ("Base low (MOVW r6,#0x0000)", "00008000:       E3006000"),
    ("Base set (MOVT r6,#0x0001)", "00008004:       E3406001"),

    # Case A: ADC r5 = r3(1) + r4(2) + C(0)  -> store r5, store flags snapshot
    ("ADC A",   "00008030:       E0B35004"),
    ("Store A r5", "00008038:       E5865000"),
    ("Store A PSR","0000803C:       E5867004"),

    # Case B: set C=0 via SUBS; ADC r5 = ~0 + 1 + C(0) -> wrap, C=1, Z=1
    ("Set C=0",     "00008040:       E250C001"),
    ("ADC B",       "0000804C:       E0B35004"),
    ("Store B r5",  "00008054:       E5865008"),
    ("Store B PSR", "00008058:       E586700C"),

    # Case C: ADC r5 = 0x7FFFFFFF + 0 + C(0)
    ("ADC C",       "0000806C:       E0B35004"),
    ("Store C r5",  "00008074:       E5865010"),
    ("Store C PSR", "00008078:       E5867014"),

    # Case D: set C=0; ADC r5 = 0x80000000 + 0x80000000 + C(0) -> Z=1, C=1, V=1
    ("ADC D",       "00008090:       E0B35004"),
    ("Store D r5",  "00008098:       E5865018"),
    ("Store D PSR", "0000809C:       E586701C"),

    # Case E: ADC with shift (r5 = 5 + (1<<1) + C(prev=1) = 8)
    ("ADC E (shift)","000080AC:       E0B35084"),
    ("Store E r5",  "000080B4:       E5865020"),
    ("Store E PSR", "000080B8:       E5867024"),

    # stop
    ("BKPT",        "000080BC:       E1212374"),

    # final state
    ("Final r5",    "r5  = 0x00000008"),
    ("Final base",  "r6  = 0x00010000"),
    ("Final PC",    "r15 = 0x000080BC"),
    ("Final CPSR",  "CPSR = 0x00000000"),
    ("Cycle count=48", "cycle=48"),
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