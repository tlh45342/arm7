import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_rsc"

CHECKS = [
    # --- scaffolding ---
    ("Debug enabled", "[DEBUG] debug_flags set to 0x000003FF"),
    ("Binary loaded", "[LOAD] test_rsc.bin @ 0x00008000 (60 bytes)"),
    ("PC set",        "r15 <= 0x00008000"),

    # --- instruction trace / decode hits ---
    ("MOV r0,#2",     "00008000:       E3A00002        mov r0, #0x2"),
    ("MOV r1,#0xA",   "00008004:       E3A0100A        mov r1, #0xa"),

    ("CMP r0,r0",     "00008008:       E1500000        cmp r0, r0"),
    ("MRS -> r4",     "0000800C:       E10F4000        mrs r4, cpsr"),

    ("RSC #1",        "00008010:       E0E02001        .word 0xE0E02001"),

    ("CMP r0,r1",     "00008014:       E1500001        cmp r0, r1"),
    ("MRS -> r5",     "00008018:       E10F5000        mrs r5, cpsr"),

    ("RSC #2",        "0000801C:       E0E03001        .word 0xE0E03001"),
    ("RSCS",          "00008020:       E0F08001        .word 0xE0F08001"),
    ("MRS -> r6",     "00008024:       E10F6000        mrs r6, cpsr"),

    ("RSC shifted",   "00008028:       E0F0B081        .word 0xE0F0B081"),
    ("MRS -> r7",     "0000802C:       E10F7000        mrs r7, cpsr"),

    ("LDR literal",   "00008030:       E51FC000        ldr r12, [pc, #+0]"),
    ("LDR lit value", "[LDR lit] r12 <= [0x00008038] => 0xDEADBEEF"),

    ("BKPT",          "00008034:       E1212374        bkpt #0x1374"),

    # --- final architectural state ---
    ("Final r0..r3",  "[BKPT]r0  = 0x00000002  r1  = 0x0000000A  r2  = 0x00000008  r3  = 0x00000007"),
    ("Final r4..r7",  "r4  = 0x60000000  r5  = 0x80000000  r6  = 0x20000000  r7  = 0x20000000"),
    ("Final r8..r11", "r8  = 0x00000007  r9  = 0x00000000  r10 = 0x00000000  r11 = 0x00000012"),
    ("Final r12..r15","r12 = 0xDEADBEEF  r13 = 0x00000000  r14 = 0x00000000  r15 = 0x00008034"),
    ("Final CPSR",    "CPSR = 0x20000000  cycle=14"),
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