import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_lsl"

CHECKS = [
    # Basic load & start
    ("Binary loaded", "[LOAD] test_lsl.bin @ 0x00008000 (348 bytes)"),
    ("PC set",        "r15 <= 0x00008000"),

    # Group 0: baseline CMP
    ("G0 CMP r0,r0",  "00008008:       E1500000        cmp r0, r0"),
    ("G0 MRS r4",     "0000800C:       E10F4000        mrs r4, cpsr"),

    # Group 1: LSLS (imm #1) on various values
    ("G1 LSLS #1 on 1",          "00008018:       E10F5000        mrs r5, cpsr"),
    ("G1 LSLS #1 on 0x80000000", "00008024:       E10F6000        mrs r6, cpsr"),
    ("G1 LSLS #1 on 0x40000000", "00008030:       E10F7000        mrs r7, cpsr"),
    ("G1 LSLS #1 on 0",          "0000803C:       E10F8000        mrs r8, cpsr"),

    # Group 2: LSLS #31
    ("G2 LSLS #31 on 1", "00008048:       E10F9000        mrs r9, cpsr"),
    ("G2 LSLS #31 on 3", "00008054:       E10FA000        mrs r10, cpsr"),

    # Group 3: plain LSL (no S) – flags unchanged
    ("G3 pre-plain LSL CPSR",  "00008060:       E10FB000        mrs r11, cpsr"),
    ("G3 post-plain LSL CPSR", "00008068:       E10FC000        mrs r12, cpsr"),

    # Group 4: LSLS with register shift amounts
    ("G4 r3=0",          "00008078:       E10F1000        mrs r1, cpsr"),
    ("G4 r3=1",          "00008084:       E10F2000        mrs r2, cpsr"),
    ("G4 r3=31",         "00008090:       E10F3000        mrs r3, cpsr"),
    ("G4 r3=32 bit0=1",  "0000809C:       E10FE000        mrs r14, cpsr"),
    ("G4 r3=32 bit0=0",  "000080AC:       E10F5000        mrs r5, cpsr"),
    ("G4 r3=33",         "000080BC:       E10F6000        mrs r6, cpsr"),
    ("G4 r3=0x100",      "000080D0:       E10F7000        mrs r7, cpsr"),
    ("G4 r3=0xFF",       "000080E0:       E10F8000        mrs r8, cpsr"),

    # Group 5: conditional LSL (EQ / NE)
    ("G5 EQ cmp",            "000080EC:       E1500001        cmp r0, r1"),
    ("G5 MRS before lsleq",  "000080F0:       E10F9000        mrs r9, cpsr"),
    ("G5 lsleq executes",    "000080F8:       01A02080        .word 0x01A02080"),
    ("G5 MRS after lsleq",   "000080FC:       E10FA000        mrs r10, cpsr"),
    ("G5 MRS before lslne",  "0000810C:       E10FB000        mrs r11, cpsr"),
    ("G5 lslne (should be skipped)", "00008114:       11A02080        .word 0x11A02080"),
    ("G5 MRS after lslne",   "00008118:       E10FC000        mrs r12, cpsr"),

    # Group 6: pattern tests
    ("G6 LSLS on 3",          "00008124:       E10F1000        mrs r1, cpsr"),
    ("G6 LSLS on 0x80000000", "00008130:       E10F2000        mrs r2, cpsr"),
    ("G6 LSLS on 0xF0000001", "0000813C:       E10F3000        mrs r3, cpsr"),

    # Final sanity + termination
    ("Final CMP lr,lr",      "00008140:       E15E000E        cmp r14, r14"),
    ("BKPT decoded",         "[K12] BKPT match (key=0x127)"),
    ("DEADBEEF in r13",      "r13 = 0xDEADBEEF"),
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