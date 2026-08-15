import subprocess
import os
import sys

VM = "arm-vm.exe"

TEST_NAME = "basic"

CHECKS = [
    # Setup / header
    ("Debug flags", "[DEBUG] debug_flags set to 0x000003FF"),
    ("Version",     "arm-vm version 0.0.144"),
    ("LOAD line",   "[LOAD] basic.bin @ 0x00008000 (36 bytes)"),
    ("PC start",    "r15 <= 0x00008000"),

    # Memory dump around the code + DEADBEEF
    ("Mem 0x8000",  "0x00008000: 01 66 a0 e3 14 00 9f e5 00 00 86 e5 00 10 96 e5"),
    ("Mem 0x8010",  "0x00008010: 42 20 a0 e3 04 20 c6 e5 04 20 d6 e5 70 00 20 e1"),
    ("Mem 0x8020",  "0x00008020: ef be ad de 00 00 00 00 00 00 00 00 00 00 00 00"),

    # instr 1: MOV r6, #0x100000
    ("DISASM 1",    "00008000:       E3A06601        mov r6, #0x100000"),
    ("TRACE 1",     "[TRACE] PC=0x00008000 Instr=0xE3A06601"),
    ("K12 key 1",   "[K12] key=0x3A0 op1=1 op2=26 op3=0"),
    ("K12 match 1", "[K12] MOV (imm) match (key=0x3A0)"),

    # instr 2: LDR r0, [pc, #+20]  (literal load of DEADBEEF)
    ("DISASM 2",    "00008004:       E59F0014        ldr r0, [pc, #+20]"),
    ("TRACE 2",     "[TRACE] PC=0x00008004 Instr=0xE59F0014"),
    ("K12 key 2",   "[K12] key=0x591 op1=2 op2=25 op3=1"),
    ("K12 match 2", "[K12] LDR(literal) match (key=0x591)"),
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