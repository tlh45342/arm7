import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_disk_read2"

CHECKS = [
    # --- Early decodes (front of program) ---
    ("MOVW decoded (early)", "[K12] MOVW match (key=0x300)"),
    ("MOVT decoded (early)", "[K12] MOVT match (key=0x341)"),

    # --- ASR instruction detection (K12 op buckets you use for ASR forms) ---
    # Treat these as your ASR test ops (register/imm variants)
    ("ASR form A present",   "[K12] MOV match (key=0x1BC)"),  # e.g., E1B050C3 / E1B05FC3
    ("ASR form B present",   "[K12] MOV match (key=0x1B0)"),  # e.g., E1B05003
    ("ASR form C present",   "[K12] MOV match (key=0x1B5)"),  # e.g., E1B05453

    # --- ASR results (sign extension behavior via observable stores) ---
    # After first ASR pair
    ("ASR result #1 r5",     "mem[0x00100000] <= r5 (0xC0000000)"),
    ("ASR result #1 r7",     "mem[0x00100004] <= r7 (0xA0000000)"),
    # After second ASR pair
    ("ASR result #2 r5",     "mem[0x00100008] <= r5 (0xFFFFFFFF)"),
    ("ASR result #2 r7",     "mem[0x0010000C] <= r7 (0x80000000)"),
    # Mixed-edge cases showing sign propagation and boundary behavior
    ("ASR result #3 r5",     "mem[0x00100010] <= r5 (0x80000001)"),
    ("ASR result #3 r7",     "mem[0x00100014] <= r7 (0x80000000)"),
    ("ASR result #4 r5",     "mem[0x00100018] <= r5 (0x3FFFFFFF)"),
    ("ASR result #4 r7",     "mem[0x0010001C] <= r7 (0x00000000)"),

    # --- Halting condition ---
    ("DEADBEEF trap",        "[K12] DEADBEEF match"),
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