import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_cmp"

CHECKS = [
    # Basic startup
    ("load",         "[LOAD] test_cmp.bin @ 0x00008000"),
    ("pc_start",     "r15 <= 0x00008000"),

    # Case 1: cmp r1, r2  → MRS r0, CPSR → STR to [r6,#0]
    ("cmp1_op",      "00008008:       E1510002"),
    ("cmp1_k12",     "[K12] CMP match (key=0x150)"),
    ("cmp1_mrs",     "0000800C:       E10F0000        mrs r0, cpsr"),
    ("cmp1_str",     "[STR pre-imm] [0x00000000] <= r0 (0x60000000) "),

    # Case 2: cmp r0, #0x2 → MRS r1, CPSR → STR to [r6,#4]
    ("cmp2_op",      "00008018:       E3500002        cmp r0, #0x2"),
    ("cmp2_k12",     "[K12] CMP match (key=0x350)"),
    ("cmp2_mrs",     "0000801C:       E10F1000        mrs r1, cpsr"),
    ("cmp2_str",     "[STR pre-imm] [0x00000004] <= r1 (0x80000000) "),

    # Case 3: cmp r0, #0x1 (with 0x80000000) → MRS r2, CPSR → STR to [r6,#8]
    ("cmp3_op",      "0000802C:       E3500001        cmp r0, #0x1"),
    ("cmp3_k12",     "[K12] CMP match (key=0x350)"),
    ("cmp3_mrs",     "00008030:       E10F2000        mrs r2, cpsr"),
    ("cmp3_str",     "[STR pre-imm] [0x00000008] <= r2 (0x30000000) "),

    # Case 4: cmp r2, r3, lsl #4 → MRS r4, CPSR → STR to [r6,#12]
    ("cmp4_op",      "00008040:       E1520203"),
    ("cmp4_k12",     "[K12] CMP match (key=0x150)"),
    ("cmp4_mrs",     "00008044:       E10F4000        mrs r4, cpsr"),
    ("cmp4_str",     "[STR pre-imm] [0x0000000C] <= r4 (0x60000000) "),

    # Case 5: mvn r4,#0; cmp r4,#0 → MRS r5, CPSR → STR to [r6,#16]
    ("mvn_op",       "0000804C:       E3E04000"),
    ("mvn_k12",      "[K12] MVN (imm) match (key=0x3E0)"),
    ("cmp5_op",      "00008050:       E3540000        cmp r4, #0x0"),
    ("cmp5_k12",     "[K12] CMP match (key=0x350)"),
    ("cmp5_mrs",     "00008054:       E10F5000        mrs r5, cpsr"),
    ("cmp5_str",     "[STR pre-imm] [0x00000010] <= r5 (0xA0000000) "),

    # Final BKPT and register dump
    ("bkpt_op",      "0000805C:       E1212374        bkpt #0x1374"),
    ("bkpt_tag",     "[BKPT]"),
    ("final_cpsr",   "CPSR = 0xA0000000  cycle=24"),
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