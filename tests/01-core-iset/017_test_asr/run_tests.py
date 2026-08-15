import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_asr"

CHECKS = [
    # Harness checks
    ("Binary loaded", "[LOAD] test_asr.bin @ 0x00008000"),
    ("PC set",        "r15 <= 0x00008000"),

    # Begin ASR tests
    ("MOV lr,#0",       "00008000:       E3A0E000"),
    ("MOV r0,#0",       "00008004:       E3A00000"),
    ("ASRS #1",         "00008008:       E1B010C0"),
    ("MRS -> r2(1)",    "0000800C:       E10F2000"),

    ("MOVW r0,#0",      "00008010:       E3000000"),
    ("MOVT r0,#0x8000", "00008014:       E3480000"),
    ("ASRS #1(b)",      "00008018:       E1B010C0"),
    ("MRS -> r2(2)",    "0000801C:       E10F2000"),

    ("MOVW r0,#0(2)",   "00008020:       E3000000"),
    ("MOVT r0,#0x8000(2)", "00008024:       E3480000"),
    ("ASRS #31",        "00008028:       E1B01FC0"),
    ("MRS -> r2(3)",    "0000802C:       E10F2000"),

    ("MOVW r0,#0(3)",   "00008030:       E3000000"),
    ("MOVT r0,#0x1000", "00008034:       E3410000"),
    ("ASRS #31(b)",     "00008038:       E1B01FC0"),
    ("MRS -> r2(4)",    "0000803C:       E10F2000"),

    ("MOVW r0,#0(4)",   "00008040:       E3000000"),
    ("MOVT r0,#0x8000(3)", "00008044:       E3480000"),
    ("ASRS #32",        "00008048:       E1B01040"),
    ("MRS -> r2(5)",    "0000804C:       E10F2000"),

    ("MOVW r0,#0(5)",   "00008050:       E3000000"),
    ("MOVT r0,#0x1000(2)", "00008054:       E3410000"),
    ("ASRS #32(b)",        "00008058:       E1B01040"),
    ("MRS -> r2(6)",       "0000805C:       E10F2000"),

    # CMP baseline
    ("CMP lr,lr",          "00008060:       E15E000E"),
    ("MRS -> r3(cmp1)",    "00008064:       E10F3000"),

    # Mixed ASR immediate/reg shifts
    ("MOVW #0x5678",      "00008068:       E3050678"),
    ("MOVT #0x1234",      "0000806C:       E3410234"),
    ("MOV r2,#0",         "00008070:       E3A02000"),
    ("ASRS r0,r2",        "00008074:       E1B01250"),
    ("MRS -> r3(shift0)", "00008078:       E10F3000"),

    ("MOV r2,#8",       "0000807C:       E3A02008"),
    ("ASRS r0,#8",      "00008080:       E1B01250"),
    ("MRS -> r3(shift8)", "00008084:       E10F3000"),

    ("MOVW r0,#0",      "00008088:       E3000000"),
    ("MOVT r0,#0x8000", "0000808C:       E3480000"),
    ("MOV r2,#32",      "00008090:       E3A02020"),
    ("ASRS r0,#32",     "00008094:       E1B01250"),
    ("MRS -> r3(shift32)", "00008098:       E10F3000"),

    ("MOVW r0,#0(again)", "0000809C:       E3000000"),
    ("MOVT r0,#0x8000(again)", "000080A0:       E3480000"),
    ("MOV r2,#40",      "000080A4:       E3A02028"),
    ("ASRS r0,#40",     "000080A8:       E1B01250"),
    ("MRS -> r3(shift40)", "000080AC:       E10F3000"),

    ("MOVW r0,#0(4096)", "000080B0:       E3000000"),
    ("MOVT r0,#4096",   "000080B4:       E3410000"),
    ("MOV r2,#40(b)",   "000080B8:       E3A02028"),
    ("ASRS r0,#40(b)",  "000080BC:       E1B01250"),
    ("MRS -> r3(shift40b)", "000080C0:       E10F3000"),

    # CMP again
    ("CMP lr,lr(2)",    "000080C4:       E15E000E"),
    ("MRS -> r3(cmp2)", "000080C8:       E10F3000"),

    # More ASR tests
    ("MOVW #0x9ABC",       "000080CC:       E3090ABC"),
    ("MOVT #0xDEF0",       "000080D0:       E34D0EF0"),

    ("MOV r2,#0",          "000080D4:       E3A02000"),
    ("ORR r2,#0x100",      "000080D8:       E3822C01"),
    ("ASRS r0,r2 (0x100)", "000080DC:       E1B01250"),
    ("MRS -> r3(asr100)",  "000080E0:       E10F3000"),

    ("MOVW #0x7FFF",    "000080E4:       E3070FFF"),
    ("MOVT #0x0000",    "000080E8:       E3400000"),
    ("MOV r2,#0xFF",    "000080EC:       E3A020FF"),
    ("ASRS r0,#255",    "000080F0:       E1B01250"),
    ("MRS -> r3(asr255)", "000080F4:       E10F3000"),

    ("CMP lr,lr(3)",    "000080F8:       E15E000E"),
    ("MRS -> r4(cmp3)", "000080FC:       E10F4000"),

    # Final block
    ("MOVW #0xAAAA",    "00008100:       E30A0AAA"),
    ("MOVT #0x5555",    "00008104:       E3450555"),
    ("ASR #1 simple",   "00008108:       E1A010C0"),
    ("MRS -> r5(asr1)", "0000810C:       E10F5000"),

    ("MOV r0,#1",       "00008110:       E3A00001"),
    ("ASRS #1 again",   "00008114:       E1B010C0"),
    ("MRS -> r6(asr1b)", "00008118:       E10F6000"),

    ("MOVW #0",         "0000811C:       E3000000"),
    ("MOVT #0x8000 end","00008120:       E3480000"),
    ("ASRS #1 last1",   "00008124:       E1B010C0"),
    ("MRS -> r6(last)", "00008128:       E10F6000"),

    ("MOVW r5,#1",      "0000812C:       E3005001"),
    ("MOVT r5,#0x8000", "00008130:       E3485000"),
    ("ASRS r5,#1",      "00008134:       E1B050C5"),
    ("MRS -> r7(asr5)", "00008138:       E10F7000"),

    ("CMP lr,lr final",   "0000813C:       E15E000E"),
    ("ASREQ r8,#1",       "00008140:       01A080C5"),
    ("CMP lr,r0",         "00008144:       E15E0000"),
    ("ASREQ r8,#1(2)",    "00008148:       01A080C5"),
    ("MRS -> r9(final)",  "0000814C:       E10F9000"),

    # Literal + BKPT + final registers
    ("LDR literal",       "00008150:       E51FD000"),
    ("BKPT",              "00008154:       E1212374"),

    # Final register snapshot includes cycle count
    ("Final r13=DEADBEEF", "r13 = 0xDEADBEEF"),
    ("Final CPSR",       "CPSR = 0x90000000"),
    ("Final cycle",      "cycle=86"),
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