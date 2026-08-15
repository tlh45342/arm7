import subprocess
import os
import sys

VM = "arm-vm.exe"

TEST_NAME = "test_ldrd"

CHECKS = [
    ("8000 movw r0, #0xF0",                 "00008000:       E30000F0"),
    ("8004 movt r0, #0xF0F0",               "00008004:       E34F00F0"),
    ("8008 movw r1, #0x0F0F",               "00008008:       E3001F0F"),
    ("800C movt r1, #0x0F0F",               "0000800C:       E3401F0F"),
    ("8010 and r2, r0, r1",                  "00008010:       E0002001"),
    ("8014 ands r3, r0, r0",                 "00008014:       E0103000"),
    ("8018 and r4, r0, #0xFF000000",         "00008018:       E20044FF"),
    ("801C movw r1, #0x1",                   "0000801C:       E3001001"),
    ("8020 movt r1, #0x8000",                "00008020:       E3481000"),
    ("8024 mov r2, #0x1",                    "00008024:       E3A02001"),
    ("8028 ands r5, r1, r1, lsr r2",         "00008028:       E0115231"),
    ("802C ands r6, r0, #0x40000000",        "0000802C:       E2106101"),
    ("8030 ands r7, r0, r0",                 "00008030:       E0107000"),
    ("8034 ands r8, r1, r1",                 "00008034:       E0118001"),
    ("8038 and r0, r0, r1",                  "00008038:       E0000001"),
    ("803C mov r9, #0x0",                    "0000803C:       E3A09000"),
    ("8040 tst r9, r9",                       "00008040:       E1190009"),
    ("8044 andne r10, r0, r0",               "00008044:       1000A000"),  # not taken
    ("8048 HALT (.word DEADBEEF)",           "00008048:       DEADBEEF"),

    # Halt + final state
    ("HALT",        "[HALT] DEADBEEF.  Halting VM."),
    ("Reg r2=1",    "r2  = 0x00000001"),
    ("CPSR Z=1",    "CPSR = 0x40000000"),
    ("Cycle count", "cycle=19"),
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