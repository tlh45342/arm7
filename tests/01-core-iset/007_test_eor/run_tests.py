import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_eor"

CHECKS = [
    # --- Startup ---
    ("load",         "[LOAD] test_eor.bin @ 0x00008000"),
    ("pc_start",     "r15 <= 0x00008000"),

    # --- Case 1: eor r0, r1, r2 -> store result at [r6,#0] ---
    ("eor1_op",      "00008018:       E0210002        eor r0, r1, r2"),
    ("eor1_k12",     "[K12] EOR match (key=0x020)"),
    ("eor1_store",   "[STR pre-imm] [0x00000000] <= r0 (0xB89EFCD2) "),

    # --- Case 2: eors r5, r3, r4, lsr #1 -> CPSR & result ---
    ("eor2_op",      "00008028:       E03350A4"),
    ("eor2_k12",     "[K12] EOR match (key=0x03A)"),
    ("eor2_mrs",     "0000802C:       E10F7000        mrs r7, cpsr"),
    ("eor2_cpsr",    "[STR pre-imm] [0x00000004] <= r7 (0x60000000) "),
    ("eor2_res",     "[STR pre-imm] [0x00000008] <= r5 (0x00000000) "),

    # --- Case 3: eor with immediates (#0x80 and #0x80000000) ---
    ("eor3_op1",     "00008038:       E2218080"),
    ("eor3_store1",  "[STR pre-imm] [0x0000000C] <= r8 (0x123456F8) "),
    ("eor3_op2",     "00008040:       E2219102        eor r9, r1, #0x80000000"),
    ("eor3_store2",  "[STR pre-imm] [0x00000010] <= r9 (0x92345678) "),

    # --- Case 4: eors r10, r3, r4, asr #1 -> CPSR & result ---
    ("eor4_op",      "00008054:       E033A0C4        eors r10, r3, r4, asr #1"),
    ("eor4_k12",     "[K12] EOR match (key=0x03C)"),
    ("eor4_mrs",     "00008058:       E10FB000        mrs r11, cpsr"),
    ("eor4_res",     "[STR pre-imm] [0x00000014] <= r10 (0xC0000000) "),
    ("eor4_cpsr",    "[STR pre-imm] [0x00000018] <= r11 (0xA0000000) "),

    # --- Final BKPT + summary state ---
    ("bkpt_op",      "00008064:       E1212374"),
    ("bkpt_tag",     "[BKPT]"),
    ("final_r0",     "r0  = 0xB89EFCD2"),
    ("final_r1",     "r1  = 0x12345678"),
    ("final_r2",     "r2  = 0xAAAAAAAA"),
    ("final_r8",     "r8  = 0x123456F8"),
    ("final_r9",     "r9  = 0x92345678"),
    ("final_r10",    "r10 = 0xC0000000"),
    ("final_r11",    "r11 = 0xA0000000"),
    ("Final CPSR",   "CPSR = 0xA0000000"),
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