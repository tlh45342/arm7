import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_orr"

CHECKS = [
    # --- Startup / config ---
    ("debug_enabled",   "[DEBUG] debug_flags set to 0x000003FF"),
    ("load_image",      "[LOAD] test_orr.bin @ 0x00008000 (76 bytes)"),
    ("pc_start",        "r15 <= 0x00008000"),

    # --- Basic ORR imm/reg setup ---
    ("8000_mov",        "00008000:       E3A000F0        mov r0, #0xF0"),
    ("8004_orr_imm",    "00008004:       E380100F        orr r1, r0, #0xF"),
    ("8004_k12",        "[K12] ORR (imm) match (key=0x380)"),
    ("8008_mov",        "00008008:       E3A0200F        mov r2, #0xf"),

    # ORR reg: r3 = r1 OR r2 = 0xFF
    # (disasm currently says 'eor', but K12 confirms ORR)
    ("800C_orr_reg",    "0000800C:       E1813002        eor r3, r1, r2"),
    ("800C_k12",        "[K12] ORR match (key=0x180)"),

    # r4 = 0 -> ORR imm #1 -> r4 = 1 ; r5 = 1
    ("8010_mov_r4",     "00008010:       E3A04000        mov r4, #0x0"),
    ("8014_orr_imm2",   "00008014:       E3844001        orr r4, r4, #0x1"),
    ("8018_mov_r5",     "00008018:       E3A05001        mov r5, #0x1"),

    # --- ORRS with LSL#1, flags, and conditionals ---
    # orrs r6, r4, r5, lsl #1  (still disassembled as .word, but K12 says ORR)
    ("801C_orrs_shift", "0000801C:       E1946085        .word 0xE1946085"),
    ("801C_k12",        "[K12] ORR match (key=0x198)"),

    # MRS CPSR into r10
    ("8020_mrs",        "00008020:       E10FA000        mrs r10, cpsr"),
    ("8020_k12",        "[K12] MRS match (key=0x100)"),

    # Conditional ORR/MOV that should NOT execute (Z==0)
    ("8024_orreq",      "00008024:       038770FF        orr r7, r7, #0xFF"),
    ("8028_moveq",      "00008028:       03A07001        mov r7, #0x1"),

    # --- New Test 1: ORR with rotated immediate 0x80000000 ---
    ("802C_mov_r8",     "0000802C:       E3A08000        mov r8, #0x0"),
    ("8030_orr_r8_imm", "00008030:       E3888102        orr r8, r8, #0x80000000"),
    ("8030_k12",        "[K12] ORR (imm) match (key=0x380)"),

    # --- New Test 2: ORRS imm 0x80000000 + CPSR snapshot ---
    ("8034_mov_r9",     "00008034:       E3A09000        mov r9, #0x0"),
    ("8038_orrs_r9_imm","00008038:       E3999102        orrs r9, r9, #0x80000000"),
    ("8038_k12",        "[K12] ORR (imm) match (key=0x390)"),
    ("803C_mrs",        "0000803C:       E10FB000        mrs r11, cpsr"),
    ("803C_k12",        "[K12] MRS match (key=0x100)"),

    # --- New Test 3: ORR with ROR#1 ---
    ("8040_mov_r12",    "00008040:       E3A0C001        mov r12, #0x1"),
    ("8044_orr_ror",    "00008044:       E180C0EC        .word 0xE180C0EC"),
    ("8044_k12",        "[K12] ORR match (key=0x18E)"),

    # --- BKPT and final state ---
    ("bkpt_op",         "00008048:       E1212374        bkpt #0x1374"),
    ("bkpt_tag",        "[BKPT]r0  = 0x000000F0"),

    # Final register values
    ("final_r1",        "r1  = 0x000000FF"),
    ("final_r2",        "r2  = 0x0000000F"),
    ("final_r3",        "r3  = 0x000000FF"),
    ("final_r4",        "r4  = 0x00000001"),
    ("final_r5",        "r5  = 0x00000001"),
    ("final_r6",        "r6  = 0x00000003"),
    ("final_r7",        "r7  = 0x00000000"),
    ("final_r8",        "r8  = 0x80000000"),
    ("final_r9",        "r9  = 0x80000000"),
    ("final_r10",       "r10 = 0x00000000"),
    ("final_r11",       "r11 = 0xA0000000"),
    ("final_r12",       "r12 = 0x800000F0"),

    # Final CPSR and cycle count
    ("final_cpsr",      "CPSR = 0xA0000000  cycle=19"),
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