import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_sub"

CHECKS = [
    # scaffolding
    ("Debug enabled", "[DEBUG] debug_flags set to 0x000003FF"),
    ("Binary loaded", "[LOAD] test_sub.bin @ 0x00008000 (308 bytes)"),
    ("PC set",        "r15 <= 0x00008000"),

    # Basic CMP/MRS/SUB
    ("CMP r0,r0",     "0000800C:       E1500000"),
    ("MRS r4",        "00008010:       E10F4000"),
    ("SUB r2,#2",     "00008014:       E2402002"),
    ("MRS r5",        "00008018:       E10F5000"),

    # SUBS r3,#7
    ("SUBS r3,#7",    "0000801C:       E2503007"),
    ("MRS r6",        "00008020:       E10F6000"),

    # Zero-case (123 − 123)
    ("MOV r2,#0x7B",  "00008024:       E3A0207B"),
    ("SUBS ip,#0x7B", "00008028:       E252C07B"),
    ("MRS r7",        "0000802C:       E10F7000"),

    # Overflow case (0x80000000 − 1 via lsl)
    ("MOV r6,#1",     "00008030:       E3A06001"),
    ("LSL r6,#31",    "00008034:       E1A06F86"),
    ("SUBS sl,#1",    "00008038:       E256A001"),
    ("MRS r8",        "0000803C:       E10F8000"),

    # register SUB
    ("SUB fp,r0,r1",  "00008040:       E050B001"),
    ("MRS r9",        "00008044:       E10F9000"),

    # LSL #1 form
    ("MOV r1,#8",     "00008048:       E3A01008"),
    ("SUB LSL #1",    "0000804C:       E050B081"),
    ("MRS r9(1)",     "00008050:       E10F9000"),

    # LSR #1 form
    ("MOV r1,#1",     "00008054:       E3A01001"),
    ("LSL r1,#31",    "00008058:       E1A01F81"),
    ("SUB LSR #1",    "0000805C:       E056B0A1"),
    ("MRS r9(2)",     "00008060:       E10F9000"),

    # ASR #31 form
    ("MOV r1,#0xFF",  "00008064:       E3A010FF"),
    ("SUB ASR #31",   "00008068:       E051BFC6"),
    ("MRS r9(3)",     "0000806C:       E10F9000"),

    # ROR #4 form
    ("MOV r1,#0x12",  "00008070:       E3A01012"),
    ("SUB ROR #4",    "00008074:       E050B261"),
    ("MRS r9(4)",     "00008078:       E10F9000"),

    # Edge cases: 0–1
    ("MOV r0,#0",     "0000807C:       E3A00000"),
    ("MOV r1,#1",     "00008080:       E3A01001"),
    ("SUBS 0-1",      "00008084:       E0502001"),
    ("MRS r6(1)",     "00008088:       E10F6000"),

    # 0x80000000 − 1
    ("MOV r0,#0x102", "0000808C:       E3A00102"),
    ("MOV r1,#1",     "00008090:       E3A01001"),
    ("SUBS 80000000-1", "00008094:       E0502001"),
    ("MRS r7(ovf)",     "00008098:       E10F7000"),

    # 0x80000000 − 0x80000000
    ("MOV r0,#0x102", "0000809C:       E3A00102"),
    ("MOV r1,#0x102", "000080A0:       E3A01102"),
    ("SUBS equal",    "000080A4:       E0502001"),
    ("MRS r8(equal)", "000080A8:       E10F8000"),

    # Register-shifted-register (two tests)
    ("MOV r4,#16",    "000080AC:       E3A04010"),
    ("MOV r5,#1",     "000080B0:       E3A05001"),
    ("MOV r6,#2",     "000080B4:       E3A06002"),
    ("SUB r3,r4,r5,lsl r6", "000080B8:       E0543615"),
    ("MRS r9(regshift1)",   "000080BC:       E10F9000"),

    ("MOV r4,#2",     "000080C0:       E3A04002"),
    ("MOV r5,#4",     "000080C4:       E3A05004"),
    ("MOV r6,#1",     "000080C8:       E3A06001"),
    ("SUB r3,r4,r5,lsl r6 (neg)", "000080CC:       E0543615"),
    ("MRS sl(regshift2)",   "000080D0:       E10FA000"),

    # Conditional EQ/NE block
    ("MOV r0,#5(cond)", "000080D4:       E3A00005"),
    ("MOV r1,#8(cond)", "000080D8:       E3A01008"),
    ("CMP 5,8",         "000080DC:       E1500001"),
    ("SUBEQ (not taken)", "000080E0:       0040E001"),
    ("SUBNE (taken)",     "000080E4:       1040E001"),

    # GE/LT block #1 (5 ≥ 3)
    ("MOV r0,#5(GE1)", "000080EC:       E3A00005"),
    ("MOV r1,#3(GE1)", "000080F0:       E3A01003"),
    ("CMP 5,3",        "000080F4:       E1500001"),
    ("SUBGE taken",    "000080FC:       A0403001"),
    ("SUBLT not taken","00008100:       B0413000"),

    # GE/LT block #2 (3 < 5)
    ("MOV r0,#3(LT2)", "00008108:       E3A00003"),
    ("MOV r1,#5(LT2)", "0000810C:       E3A01005"),
    ("CMP 3,5",        "00008110:       E1500001"),
    ("SUBGE not taken","00008118:       A0403001"),
    ("SUBLT taken",    "0000811C:       B0413000"),

    # Literal + BKPT
    ("LDR literal",   "00008128:       E51FD000"),
    ("Literal value", "[LDR lit] r13 <= [0x00008130] => 0xDEADBEEF"),
    ("BKPT",          "0000812C:       E1212374"),
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