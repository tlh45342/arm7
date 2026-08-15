import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_ror"

CHECKS = [
    # --- Startup / config ---
    ("load_image",      "[LOAD] test_ror.bin @ 0x00008000 (52 bytes)"),
    ("pc_start",        "r15 <= 0x00008000"),

    # --- Base pointer setup: r6 = 0x00100000 ---
    ("movw_r6",         "00008000:       E3006000        movw r6, #0x0000"),
    ("movw_r6_k12",     "[K12] MOVW match (key=0x300)"),
    ("movt_r6",         "00008004:       E3406010        movt r6, #0x0010"),
    ("movt_r6_k12",     "[K12] MOVT match (key=0x341)"),

    # --- r0 = 0x80000001 ---
    ("movw_r0",         "00008008:       E3000001        movw r0, #0x0001"),
    ("movw_r0_k12",     "[K12] MOVW match (key=0x300)"),
    ("movt_r0",         "0000800C:       E3480000        movt r0, #0x8000"),
    ("movt_r0_k12",     "[K12] MOVT match (key=0x340)"),

    # --- First RORS (E1B010E0) + CPSR snapshot, stored to memory ---
    ("ror1_op",         "00008010:       E1B010E0        .word 0xE1B010E0"),
    ("ror1_k12",        "[K12] MOV match (key=0x1BE)"),
    ("mrs1",            "00008014:       E10F7000        mrs r7, cpsr"),
    ("mrs1_k12",        "[K12] MRS match (key=0x100)"),
    ("str_r1",          "[STR pre-imm] [0x00100000] <= r1 (0xC0000000) "),
    ("str_r7_first",    "[STR pre-imm] [0x00100004] <= r7 (0xA0000000) "),

    # --- Second RORS (E1B02460) + CPSR snapshot, stored to memory ---
    ("ror2_op",         "00008020:       E1B02460        .word 0xE1B02460"),
    ("ror2_k12",        "[K12] MOV match (key=0x1B6)"),
    ("mrs2",            "00008024:       E10F7000        mrs r7, cpsr"),
    ("mrs2_k12",        "[K12] MRS match (key=0x100)"),
    ("str_r2",          "00008028:       E5862008        str r2, [r6, #+8]"),
    ("str_r2_k12",      "[K12] STR  pre-imm match (key=0x580)"),
    ("str_r2_val",      "[STR pre-imm] [0x00100008] <= r2 (0x01800000) "),
    ("str_r7_second",   "[STR pre-imm] [0x0010000C] <= r7 (0x00000000) "),

    # --- BKPT and final state ---
    ("bkpt_op",         "00008030:       E1212374        bkpt #0x1374"),
    ("bkpt_k12",        "[K12] BKPT match (key=0x127)"),
    ("bkpt_tag",        "[BKPT]r0  = 0x80000001  r1  = 0xC0000000  r2  = 0x01800000  r3  = 0x00000000"),

    # Final registers and CPSR
    ("final_r6",        "r6  = 0x00100000"),
    ("final_r1",        "r1  = 0xC0000000"),
    ("final_r2",        "r2  = 0x01800000"),
    ("final_r7",        "r7  = 0x00000000"),
    ("final_pc",        "r15 = 0x00008030"),
    ("final_cpsr",      "CPSR = 0x00000000  cycle=13"),
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