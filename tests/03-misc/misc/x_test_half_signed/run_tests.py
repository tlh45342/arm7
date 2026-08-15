import subprocess
import os
import sys

VM = "arm-vm.exe"

TEST_NAME = "test_half_signed"

CHECKS = [
    # setup / config
    ("Debug enabled",        "[DEBUG] debug_flags set to 0x000003FF"),
    ("Loaded image",         "[LOAD] test_half_signed.bin @ 0x00008000"),
    ("PC start",             "r15 <= 0x00008000"),

    # instruction lines (address + encoding; mnemonics when present)
    ("MOV r10,#0 @8000",     "00008000:       E3A0A000        mov r10, #0x0"),
    ("MOVT r10,#0x0010",     "00008004:       E340A010        movt r10, #0x0010"),
    ("MOVW r2,#0xBEEF",      "00008008:       E30B2EEF        movw r2, #0xBEEF"),
    ("STRH enc @800C",       "0000800C:       E1EA20B2"),
    ("STRB r1,[r10,#4]",     "00008018:       E5CA1004        strb r1, [r10, #+4]"),
    ("LDRSB enc @801C",      "0000801C:       E1DA40D4"),
    ("STRH enc @8028",       "00008028:       E1CA10B6"),
    ("LDRSH enc @802C",      "0000802C:       E1DA50F6"),

    # side-effect cue
    ("STRB wrote 0x80 @+6",  "mem[0x00100006] <= r1 (0x80)"),

    # graceful stop
    ("DEADBEEF trap",        "00008030:       DEADBEEF"),

    # final machine state
    ("Final r4 signed byte", "r4  = 0xFFFFFF80"),
    ("Final r5 signed half", "r5  = 0x00000001"),
    ("Final base r10",       "r10 = 0x00100002"),
    ("Final base r3",        "r3  = 0x00100002"),
    ("Final PC",             "r15 = 0x00008030"),
    ("Final CPSR",           "CPSR = 0x00000000"),
    ("Cycle count",          "cycle=13"),
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