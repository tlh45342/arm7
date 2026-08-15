import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_tst"

CHECKS = [
    # harness checks
    ("Binary loaded", "[LOAD] test_tst.bin @ 0x00008000"),
    ("PC set",        "r15 <= 0x00008000"),

    # Basic TST cases
    ("MOV lr,#0",       "00008000:       E3A0E000"),
    ("MOV r0,#0xF0",    "00008004:       E3A000F0"),
    ("MOV r1,#0",       "00008008:       E3A01000"),
    ("TST r0,r1",       "0000800C:       E1100001"),
    ("MRS -> r4",       "00008010:       E10F4000"),

    ("MOV r0,#0xAA",    "00008014:       E3A000AA"),
    ("MOV r1,#0x0F",    "00008018:       E3A0100F"),
    ("TST r0,r1",       "0000801C:       E1100001"),
    ("MRS -> r5",       "00008020:       E10F5000"),

    # MSB/negative & shifted register setup
    ("MOV r0,#1",       "00008024:       E3A00001"),
    ("LSL r0,#31",      "00008028:       E1A00F80"),
    ("MOV r1,#0xF0",    "0000802C:       E3A010F0"),
    ("LSL r1,#24",      "00008030:       E1A01C01"),
    ("TST r0,r1",       "00008034:       E1100001"),
    ("MRS -> r6",       "00008038:       E10F6000"),

    # CMP baseline vs TST
    ("MOV r0,#5",       "0000803C:       E3A00005"),
    ("MOV r1,#3",       "00008040:       E3A01003"),
    ("CMP r1,r0",       "00008044:       E1510000"),
    ("MRS -> r7",       "00008048:       E10F7000"),

    ("TST r0,#0xFF",    "0000804C:       E31000FF"),
    ("MRS -> r8",       "00008050:       E10F8000"),

    ("CMP r0,r0",       "00008054:       E1500000"),
    ("MRS -> r9",       "00008058:       E10F9000"),

    ("TST r0,#0x0F",    "0000805C:       E310000F"),
    ("MRS -> sl",       "00008060:       E10FA000"),

    ("TST r0,#0x80000000", "00008064:       E3100102"),
    ("MRS -> fp",       "00008068:       E10FB000"),

    ("TST r0,#0x20000000", "0000806C:       E3100202"),
    ("MRS -> ip",       "00008070:       E10FC000"),

    # Shifted register (imm shifts)
    ("MOV r2,#1",       "00008074:       E3A02001"),
    ("LSL r2,#31",      "00008078:       E1A02F82"),
    ("TST r0,r2,lsl#1", "0000807C:       E1100082"),
    ("MRS -> r3(1)",    "00008080:       E10F3000"),

    ("MOV r2,#1(2)",    "00008084:       E3A02001"),
    ("TST r0,r2,lsr#1", "00008088:       E11000A2"),
    ("MRS -> r3(2)",    "0000808C:       E10F3000"),

    ("MOV r2,#0",       "00008090:       E3A02000"),
    ("MVN r2,r2",       "00008094:       E1E02002"),
    ("TST r0,r2,asr#31","00008098:       E1100FC2"),
    ("MRS -> r3(3)",    "0000809C:       E10F3000"),

    ("MOV r2,#1(3)",    "000080A0:       E3A02001"),
    ("TST r0,r2,rrx",   "000080A4:       E1100062"),
    ("MRS -> r3(4)",    "000080A8:       E10F3000"),

    # Reg-shift-reg TST (lsl r2, lsr r2)
    ("MOV r2,#1(4)",    "000080AC:       E3A02001"),
    ("MOV r3,#1",       "000080B0:       E3A03001"),
    ("LSL r3,#31",      "000080B4:       E1A03F83"),
    ("TST r0,r3,lsl r2","000080B8:       E1100213"),
    ("MRS -> r1(1)",    "000080BC:       E10F1000"),

    ("MOV r2,#0x1F",    "000080C0:       E3A0201F"),
    ("MOV r3,#1(2)",    "000080C4:       E3A03001"),
    ("LSL r3,#31(2)",   "000080C8:       E1A03F83"),
    ("TST r0,r3,lsr r2","000080CC:       E1100233"),
    ("MRS -> r1(2)",    "000080D0:       E10F1000"),

    # ADDS + TST #0xF0
    ("MOV r2,#0x7F",    "000080D4:       E3A0207F"),
    ("LSL r2,#24",      "000080D8:       E1A02C02"),
    ("ADDS r2,#0x81000000", "000080DC:       E2922481"),
    ("MRS -> r0(ADDS)", "000080E0:       E10F0000"),
    ("TST r2,#0xF0",    "000080E4:       E31200F0"),
    ("MRS -> r0(TST)",  "000080E8:       E10F0000"),

    # Conditional TST (tsteq / tstne)
    ("MOV r0,#5",       "000080EC:       E3A00005"),
    ("MOV r1,#8",       "000080F0:       E3A01008"),
    ("CMP r0,r1",       "000080F4:       E1500001"),
    ("TSTEQ r0,r1",     "000080F8:       01100001"),
    ("MRS -> sp(1)",    "000080FC:       E10FD000"),
    ("CMP r0,r0",       "00008100:       E1500000"),
    ("TSTNE r0,r1",     "00008104:       11100001"),
    ("MRS -> sp(2)",    "00008108:       E10FD000"),

    # Literal load + BKPT
    ("LDR literal",     "0000810C:       E51FD000"),
    ("BKPT",            "00008110:       E1212374"),
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