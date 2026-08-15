import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_msr"

CHECKS = [
    # Harness checks
    ("Binary loaded", "[LOAD] test_msr.bin @ 0x00008000"),
    ("PC set",        "r15 <= 0x00008000"),

    # G0: initial snapshot
    ("G0 MOV lr,#0",  "00008000:       E3A0E000        mov r14, #0x0"),
    ("G0 MRS r0",     "00008004:       E10F0000        mrs r0, cpsr"),

    # G1: MSR imm -> CPSR (0x60000000) then MRS r1
    ("G1 MSR imm 0x60000000",  "00008008:       E328F206        .word 0xE328F206"),
    ("G1 MSR imm decode",      "[K12] MSR imm->CPSR match (key=0x320)"),
    ("G1 MRS r1",              "0000800C:       E10F1000        mrs r1, cpsr"),

    # G2: MSR imm -> CPSR (0x80000000) then MRS r2
    ("G2 MSR imm 0x80000000",  "00008010:       E328F102        .word 0xE328F102"),
    ("G2 MRS r2",              "00008014:       E10F2000        mrs r2, cpsr"),

    # G3: MSR reg (sl = 0xA0000000) then MRS r3
    ("G3 MOV sl,#0xA0000000",  "00008018:       E3A0A20A        mov r10, #0x20a"),
    ("G3 MSR reg sl opcode",   "0000801C:       E128F00A        .word 0xE128F00A"),
    ("G3 MSR reg decode",      "[K12] MSR reg->CPSR match (key=0x120)"),
    ("G3 MRS r3",              "00008020:       E10F3000        mrs r3, cpsr"),

    # G4: MSR imm 0 -> CPSR then MRS r4
    ("G4 MSR imm 0",           "00008024:       E328F000        .word 0xE328F000"),
    ("G4 MRS r4",              "00008028:       E10F4000        mrs r4, cpsr"),

    # G5: MVN fp,#0 then MSR reg fp -> CPSR then MRS r5
    ("G5 MVN fp,#0",           "0000802C:       E3E0B000        mvn r11, #0x0"),
    ("G5 MSR reg fp opcode",   "00008030:       E128F00B        .word 0xE128F00B"),
    ("G5 MRS r5",              "00008034:       E10F5000        mrs r5, cpsr"),

    # G6: MSR reg r0 (restore original flags) then MRS r6
    ("G6 MSR reg r0 opcode",   "00008038:       E128F000        .word 0xE128F000"),
    ("G6 MRS r6",              "0000803C:       E10F6000        mrs r6, cpsr"),

    # Tail: literal load + BKPT
    ("LDR literal",            "00008040:       E51FD000        ldr r13, [pc, #+0]"),
    ("LDR lit decoded",        "[K12] LDR(literal) match (key=0x510)"),
    ("BKPT opcode",            "00008044:       E1212374        bkpt #0x1374"),
    ("BKPT decoded",           "[K12] BKPT match (key=0x127)"),

    # Result checks (BKPT dump)
    ("r1 CPSR 0x60000000",     "r1  = 0x60000000"),
    ("r2 CPSR 0x80000000",     "r2  = 0x80000000"),
    ("r3 CPSR 0xA0000000",     "r3  = 0xA0000000"),
    ("r4 CPSR 0x00000000",     "r4  = 0x00000000"),
    ("r5 CPSR 0xF8000000",     "r5  = 0xF8000000"),
    ("r6 CPSR restored",       "r6  = 0x00000000"),

    ("SP DEADBEEF",            "r13 = 0xDEADBEEF"),
    ("Final CPSR",             "CPSR = 0x00000000"),
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