import subprocess
import os
import sys

VM = "arm-vm.exe"

TEST_NAME = "test_ldm_stm_basic"

CHECKS = [
    # setup / config
    ("Debug enabled",            "[DEBUG] debug_flags set to 0x000003FF"),
    ("Loaded image",             "[LOAD] test_ldm_stm_basic.bin @ 0x00008000"),
    ("PC start",                 "r15 <= 0x00008000"),

    # instruction lines (address : opcode : mnemonic/operands)
    ("MOV r10,#0 @8000",         "00008000:       E3A0A000        mov r10, #0x0"),
    ("MOVT r10,#0x0010",         "00008004:       E340A010        movt r10, #0x0010"),
    ("MOV r0,#0x11",             "00008008:       E3A00011        mov r0, #0x11"),
    ("MOV r1,#0x22",             "0000800C:       E3A01022        mov r1, #0x22"),
    ("MOV r2,#0x33",             "00008010:       E3A02033        mov r2, #0x33"),
    ("MOV r3,#0x44",             "00008014:       E3A03044        mov r3, #0x44"),
    ("STMIA r10!,{r0-r3}",       "00008018:       E8AA000F        stmia r10!, {r0, r1, r2, r3}"),
    ("SUB r11,r10,#0x10",        "0000801C:       E24AB010        sub r11, r10, #0x10"),
    ("LDMIA r11,{r4-r7}",        "00008020:       E89B00F0        ldmia r11, {r4, r5, r6, r7}"),

    # graceful stop
    ("DEADBEEF trap",            "00008024:       DEADBEEF"),

    # final machine state
    ("Final r4",                 "r4  = 0x00000011"),
    ("Final r5",                 "r5  = 0x00000022"),
    ("Final r6",                 "r6  = 0x00000033"),
    ("Final r7",                 "r7  = 0x00000044"),
    ("Final r10 base+16",        "r10 = 0x00100010"),
    ("Final r11 base",           "r11 = 0x00100000"),
    ("Final PC",                 "r15 = 0x00008024"),
    ("Final CPSR",               "CPSR = 0x00000000"),
    ("Cycle count",              "cycle=10"),
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