import subprocess
import os
import sys

VM = "arm-vm.exe"

TEST_NAME = "test_ldm_stm"

CHECKS = [
    # setup / config
    ("Debug enabled",            "[DEBUG] debug_flags set to 0x000003FF"),
    ("Loaded image",             "[LOAD] test_ldm_stm.bin @ 0x00008000"),
    ("PC start",                 "r15 <= 0x00008000"),

    # instruction lines (exact disasm text)
    ("LDR literal @8000",        "00008000:       E59F001C        ldr r0, [pc, #+28]"),
    ("MOV r4,#0x11",             "00008004:       E3A04011        mov r4, #0x11"),
    ("MOV r5,#0x22",             "00008008:       E3A05022        mov r5, #0x22"),
    ("MOV r6,#0x33",             "0000800C:       E3A06033        mov r6, #0x33"),
    ("STMIA r0!,{r4-r6}",        "00008010:       E8A00070        stmia r0!, {r4, r5, r6}"),
    ("SUB r1,r0,#0xC",           "00008014:       E240100C        sub r1, r0, #0xC"),
    ("LDMIA r1,{r7-r9}",         "00008018:       E8910380        ldmia r1, {r7, r8, r9}"),
    ("MOV r10,r0 (enc)",         "0000801C:       E1A0A000        .word 0xE1A0A000"),
    ("DEADBEEF trap",            "00008020:       DEADBEEF        .word 0xDEADBEEF"),

    # final machine state
    ("Final r7",                 "r7  = 0x00000011"),
    ("Final r8",                 "r8  = 0x00000022"),
    ("Final r9",                 "r9  = 0x00000033"),
    ("Final r0 post-inc",        "r0  = 0x00008034"),
    ("Final r1 base",            "r1  = 0x00008028"),
    ("Final r10 mirror r0",      "r10 = 0x00008034"),
    ("Final PC",                 "r15 = 0x00008020"),
    ("Final CPSR",               "CPSR = 0x00000000"),
    ("Cycle count",              "cycle=9"),
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