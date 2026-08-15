import subprocess
import os
import sys

VM = "arm7-run.exe"

TEST_NAME = "test_and"

CHECKS = [
    ("movw r0, #0x00F0", "00008000:       E30000F0"),
    ("movt r0, #0xF0F0", "00008004:       E34F00F0"),
    ("movw r1, #0x0F0F", "00008008:       E3001F0F"),
    ("movt r1, #0x0F0F", "0000800C:       E3401F0F"),
    ("and r2, r0, r1", "00008010:       E0002001"),
    ("ands r3, r0, r0", "00008014:       E0103000"),
    ("and r4, r0, #0xFF000000", "00008018:       E20044FF"),
    ("movw r1, #0x0001", "0000801C:       E3001001"),
    ("movt r1, #0x8000", "00008020:       E3481000"),
    ("mov r2, #0x1", "00008024:       E3A02001"),
    ("ands r5, r1, r1, lsr r2", "00008028:       E0115231"),
    ("ands r6, r0, #0x40000000", "0000802C:       E2106101"),
    ("ands r7, r0, r0", "00008030:       E0107000"),
    ("ands r8, r1, r1", "00008034:       E0118001"),
    ("and r0, r0, r1", "00008038:       E0000001"),
    ("mov r9, #0x0", "0000803C:       E3A09000"),
    ("tst r9, r9", "00008040:       E1190009"),
    ("andne r10, r0, r0", "00008044:       1000A000"),
    ("mov r11, #0x0", "00008048:       E3A0B000"),
    ("ands r12, r11, r11", "0000804C:       E01BC00B"),
    ("mov r11, #0x80000000", "00008050:       E3A0B102"),
    ("ands r13, r11, r11", "00008054:       E01BD00B"),
    ("and r14, r0, #0xFF", "00008058:       E200E0FF"),
    ("ands r15, r0, #0x80000000", "0000805C:       E210F102"),
    ("bkpt #0x1374", "00008060:       E1212374"),
    ("Cycle count",          "cycle=25"),
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