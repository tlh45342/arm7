import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_teq"

CHECKS = [
    # harness checks
    ("Binary loaded", "[LOAD] test_teq.bin @ 0x00008000"),
    ("PC set",        "r15 <= 0x00008000"),

    # Equal TEQ
    ("MOV r0,#0xF0",   "00008000:       E3A000F0"),
    ("MOV r1,#0xF0",   "00008004:       E3A010F0"),
    ("TEQ equal",      "00008008:       E1300001"),
    ("MRS -> r4",      "0000800C:       E10F4000"),

    # Non-equal TEQ
    ("MOV r0,#0xAA",   "00008010:       E3A000AA"),
    ("MOV r1,#0x0F",   "00008014:       E3A0100F"),
    ("TEQ non-equal",  "00008018:       E1300001"),
    ("MRS -> r5",      "0000801C:       E10F5000"),

    # MSB/N=1 case
    ("MOV r0,#1",      "00008020:       E3A00001"),
    ("LSL r0,#31",     "00008024:       E1A00F80"),
    ("MOV r1,#0",      "00008028:       E3A01000"),
    ("TEQ MSB",        "0000802C:       E1300001"),
    ("MRS -> r6",      "00008030:       E10F6000"),

    # CMP baseline
    ("CMP r0,r0",      "00008034:       E1500000"),
    ("MRS -> r7",      "00008038:       E10F7000"),

    # TEQ #0xFF
    ("TEQ #0xFF",      "0000803C:       E33000FF"),
    ("MRS -> r8",      "00008040:       E10F8000"),

    # TEQ #0x80000000
    ("TEQ rot imm",    "00008044:       E3300102"),
    ("MRS -> r9",      "00008048:       E10F9000"),

    # TEQ LSL #1
    ("MOV r1,#1",      "0000804C:       E3A01001"),
    ("LSL r1,#31",     "00008050:       E1A01F81"),
    ("TEQ LSL #1",     "00008054:       E1300081"),
    ("MRS -> sl",      "00008058:       E10FA000"),

    # TEQ LSR #32
    ("MOV r1,#1(2)",   "0000805C:       E3A01001"),
    ("LSL r1,#31(2)",  "00008060:       E1A01F81"),
    ("TEQ LSR #32",    "00008064:       E1300021"),
    ("MRS -> fp",      "00008068:       E10FB000"),

    # TEQ ASR #32
    ("MOV r1,#0",      "0000806C:       E3A01000"),
    ("TEQ ASR #32",    "00008070:       E1300041"),
    ("MRS -> ip",      "00008074:       E10FC000"),

    # TEQ RRX
    ("MOV r1,#1(3)",   "00008078:       E3A01001"),
    ("TEQ RRX",        "0000807C:       E1300061"),
    ("MRS -> lr",      "00008080:       E10FE000"),

    # reg-shift-reg
    ("MOV r0,#0x0F",   "00008084:       E3A0000F"),
    ("MOV r1,#1",      "00008088:       E3A01001"),
    ("MOV r2,#3",      "0000808C:       E3A02003"),
    ("TEQ lsl r2",     "00008090:       E1300211"),
    ("MRS -> r10",     "00008094:       E10FA000"),

    # ADDS + TEQ #0
    ("MOV r2,#0x7F",   "00008098:       E3A0207F"),
    ("LSL r2,#24",     "0000809C:       E1A02C02"),
    ("ADDS r2,#0x81000000", "000080A0:       E2922481"),
    ("MRS -> r3",      "000080A4:       E10F3000"),
    ("TEQ r2,#0",      "000080A8:       E3320000"),
    ("MRS -> r2",      "000080AC:       E10F2000"),

    # Conditional TEQEQ
    ("CMP for cond",   "000080B0:       E1500001"),
    ("TEQEQ r0,r1",    "000080B4:       01300001"),
    ("MRS -> r1",      "000080B8:       E10F1000"),

    # Literal load + BKPT
    ("LDR literal",    "000080BC:       E51FD000"),
    ("BKPT",           "000080C0:       E1212374"),
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