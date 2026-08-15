import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_mvn"

CHECKS = [
    ("MVN imm 0",         "00008000:       E3E00000"),
    ("MOV r2, #0x91",     "00008004:       E3A02091"),
    ("NOP #1",            "00008008:       E320F000"),
    ("MOV r1, #0xAA",     "0000800C:       E3A010AA"),
    ("MVN reg",           "00008010:       E1E03001"),
    ("MOV r2, #0x92",     "00008014:       E3A02092"),
    ("NOP #2",            "00008018:       E320F000"),
    ("TEQ",               "0000801C:       E328F202"),
    ("MVN imm 0xF",       "00008020:       E3F04000"),
    ("MOV r2, #0x93",     "00008024:       E3A02093"),
    ("NOP #3",            "00008028:       E320F000"),
    ("MOV r0, #1",        "0000802C:       E3A00001"),
    ("MVN shifted #1",    "00008030:       E1F050A0"),
    ("MOV r2, #0x94",     "00008034:       E3A02094"),
    ("NOP #4",            "00008038:       E320F000"),
    ("TEQ #2",            "0000803C:       E328F202"),
    ("MOV r0, #1 #2",     "00008040:       E3A00001"),
    ("MVN shifted #2",    "00008044:       E1F06060"),
    ("MOV r2, #0x95",     "00008048:       E3A02095"),
    ("NOP #5",            "0000804C:       E320F000"),
    ("MOV r0, #1 #3",     "00008050:       E3A00001"),
    ("MOV r1, #1",        "00008054:       E3A01001"),
    ("MVN shifted #3",    "00008058:       E1F07110"),
    ("MOV r2, #0x96",     "0000805C:       E3A02096"),
    ("NOP #6",            "00008060:       E320F000"),
    ("TEQ #3",            "00008064:       E328F202"),
    ("MOV r0, #0x55",     "00008068:       E3A00055"),
    ("MOV r1, #0x0",      "0000806C:       E3A01000"),
    ("MVN shifted #4",    "00008070:       E1F08110"),
    ("MOV r2, #0x97",     "00008074:       E3A02097"),
    ("NOP #7",            "00008078:       E320F000"),
    ("BKPT",              "0000807C:       E1200070"),
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