import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_stack"

CHECKS = [
    ("8000 mov r1, #0x11",          "00008000:       E3A01011"),
    ("8004 mov r2, #0x22",          "00008004:       E3A02022"),
    ("8008 mov r3, #0x33",          "00008008:       E3A03033"),

    ("800C STMDB sp!,{r1-r3}",      "0000800C:       E92D000E"),
    ("store r1 @ sp-12",            "[STR] mem[0x1ffffff0] <= 0x00000011"),
    ("store r2 @ sp-8",             "[STR] mem[0x1ffffff4] <= 0x00000022"),
    ("store r3 @ sp-4",             "[STR] mem[0x1ffffff8] <= 0x00000033"),

    ("8010 LDMIA sp!,{r4-r6}",      "00008010:       E8BD0070"),
    ("load r4",                     "[LDM] r4 <= mem[0x1FFFFFF0] => 0x00000011"),
    ("load r5",                     "[LDM] r5 <= mem[0x1FFFFFF4] => 0x00000022"),
    ("load r6",                     "[LDM] r6 <= mem[0x1FFFFFF8] => 0x00000033"),

    ("804? deadbeef",               "00008014:       DEADBEEF"),
    ("HALT",                        "[HALT] DEADBEEF.  Halting VM."),
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