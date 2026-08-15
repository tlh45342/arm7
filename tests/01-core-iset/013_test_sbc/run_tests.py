import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_sbc"

CHECKS = [
    # --- Test initialization ---
    ("debug_enabled", "[DEBUG] debug_flags set to 0x000003FF"),
    ("load_image",    "[LOAD] test_sbc.bin @ 0x00008000"),
    ("pc_start",      "r15 <= 0x00008000"),

    # --- Case 1: SBC with C=1 (cmp r0,r0) ---
    ("8000_mov_r0",   "00008000:       E3A00005        mov r0, #0x5"),
    ("8004_mov_r1",   "00008004:       E3A01003        mov r1, #0x3"),
    ("8008_cmp_r0_r0","00008008:       E1500000        cmp r0, r0"),
    ("8008_k12_cmp",  "[K12] CMP match (key=0x150)"),
    ("800C_mrs_r4",   "0000800C:       E10F4000        mrs r4, cpsr"),
    ("8010_sbc1",     "00008010:       E0C02001        .word 0xE0C02001"),
    ("8010_k12_sbc",  "[K12] SBC match (key=0x0C0)"),
    ("8014_mrs_r5",   "00008014:       E10F5000        mrs r5, cpsr"),

    # --- Case 2: SBC with C=0 (cmp r1,r0) ---
    ("8018_cmp_r1_r0","00008018:       E1510000        cmp r1, r0"),
    ("801C_sbc2",     "0000801C:       E0D03001        .word 0xE0D03001"),
    ("801C_k12_sbc",  "[K12] SBC match (key=0x0D0)"),
    ("8020_mrs_r6",   "00008020:       E10F6000        mrs r6, cpsr"),

    # --- Case 3: SBCS with equal regs (Z=1 path) ---
    ("8024_mov_r2",   "00008024:       E3A0207B        mov r2, #0x7B"),
    ("8028_mov_r3",   "00008028:       E3A0307B        mov r3, #0x7B"),
    ("802C_cmp_r2_r3","0000802C:       E1520003        cmp r2, r3"),
    ("802C_k12_cmp",  "[K12] CMP match (key=0x150)"),
    ("8030_sbc3",     "00008030:       E0D2C003        .word 0xE0D2C003"),
    ("8030_k12_sbc",  "[K12] SBC match (key=0x0D0)"),
    ("8034_mrs_r7",   "00008034:       E10F7000        mrs r7, cpsr"),

    # --- Case 4: SBC with large shift (0x80000000 - 1) ---
    ("8038_mov_r6",   "00008038:       E3A06001        mov r6, #0x1"),
    ("803C_mov_r6_lsl","0000803C:       E1A06F86        .word 0xE1A06F86"),
    ("803C_k12_mov",  "[K12] MOV match (key=0x1A8)"),
    ("8040_mov_r8",   "00008040:       E3A08001        mov r8, #0x1"),
    ("8044_cmp_r8_r8","00008044:       E1580008        cmp r8, r8"),
    ("8044_k12_cmp",  "[K12] CMP match (key=0x150)"),
    ("8048_sbc4",     "00008048:       E2D6A001        .word 0xE2D6A001"),
    ("8048_k12_sbc",  "[K12] SBC match (key=0x2D0)"),
    ("804C_mrs_r8",   "0000804C:       E10F8000        mrs r8, cpsr"),

    # --- Case 5: SBC with rotated immediate, C=1 from cmp r0,r0 ---
    ("8050_cmp_r0_r0","00008050:       E1500000        cmp r0, r0"),
    ("8054_mov_r1_8", "00008054:       E3A01008        mov r1, #0x8"),
    ("8058_sbc5",     "00008058:       E0D0B081        .word 0xE0D0B081"),
    ("8058_k12_sbc",  "[K12] SBC match (key=0x0D8)"),
    ("805C_mrs_r9",   "0000805C:       E10F9000        mrs r9, cpsr"),

    # --- Case 6: Conditional SBC (SBCEQ / SBCNE) ---
    ("8060_cmp_r0_r1","00008060:       E1500001        cmp r0, r1"),
    ("8064_sbceq",    "00008064:       00C0E001        .word 0x00C0E001"),
    ("8068_cmp2",     "00008068:       E1500000        cmp r0, r0"),
    ("806C_sbcne",    "0000806C:       10C0E001        .word 0x10C0E001"),

    # --- Literal load + BKPT ---
    ("8070_ldr_lit",  "00008070:       E51FD000        ldr r13, [pc, #+0]"),
    ("8070_k12_ldr",  "[K12] LDR(literal) match (key=0x510)"),
    ("ldr_value",     "[LDR lit] r13 <= [0x00008078] => 0xDEADBEEF"),
    ("bkpt_op",       "00008074:       E1212374        bkpt #0x1374"),
    ("bkpt_tag",      "[K12] BKPT match (key=0x127)"),

    # --- Final architectural state ---
    ("final_r0_3",    "[BKPT]r0  = 0x00000005  r1  = 0x00000008  r2  = 0x0000007B  r3  = 0x0000007B"),
    ("final_r4_7",    "r4  = 0x60000000  r5  = 0x60000000  r6  = 0x80000000  r7  = 0x60000000"),
    ("final_r8_11",   "r8  = 0x30000000  r9  = 0x80000000  r10 = 0x7FFFFFFF  r11 = 0xFFFFFFF5"),
    ("final_r12_15",  "r12 = 0x00000000  r13 = 0xDEADBEEF  r14 = 0x00000000  r15 = 0x00008074"),
    ("final_cpsr",    "CPSR = 0x60000000  cycle=30"),
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