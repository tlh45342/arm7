import subprocess
import os
import sys

VM = "arm-vm.exe"

TEST_NAME = "test_push_pop"

CHECKS = [
    # setup / config
    ("Debug enabled",        "[DEBUG] debug_flags set to 0x000003FF"),
    ("Loaded image",         "[LOAD] test_push_pop.bin @ 0x00008000"),
    ("PC start",             "r15 <= 0x00008000"),

    # disassembly lines
    ("MOVW r4,#0x1111",      "00008000:       E3014111        movw r4, #0x1111"),
    ("MOVT r4,#0x1111",      "00008004:       E3414111        movt r4, #0x1111"),
    ("MOVW r5,#0x2222",      "00008008:       E3025222        movw r5, #0x2222"),
    ("MOVT r5,#0x2222",      "0000800C:       E3425222        movt r5, #0x2222"),
    ("MOVW r6,#0x3333",      "00008010:       E3036333        movw r6, #0x3333"),
    ("MOVT r6,#0x3333",      "00008014:       E3436333        movt r6, #0x3333"),
    ("MOVW r7,#0x4444",      "00008018:       E3047444        movw r7, #0x4444"),
    ("MOVT r7,#0x4444",      "0000801C:       E3447444        movt r7, #0x4444"),
    ("MOV r8,sp (encoded)",  "00008020:       E1A0800D        .word 0xE1A0800D"),

    ("STMDB sp!",            "00008024:       E92D40F0        stmdb sp!, {r4, r5, r6, r7, lr}"),
    ("Zero r4",              "00008028:       E3A04000        mov r4, #0x0"),
    ("Zero r5",              "0000802C:       E3A05000        mov r5, #0x0"),
    ("Zero r6",              "00008030:       E3A06000        mov r6, #0x0"),
    ("Zero r7",              "00008034:       E3A07000        mov r7, #0x0"),
    ("LDMIA sp!",            "00008038:       E8BD40F0        ldmia sp!, {r4, r5, r6, r7, lr}"),
    ("SUB check (encoded)",  "0000803C:       E04D9008        .word 0xE04D9008"),

    # graceful stop
    ("DEADBEEF trap",        "00008040:       DEADBEEF"),

    # final machine state
    ("Final r4",             "r4  = 0x00000000"),
    ("Final r5",             "r5  = 0x44444444"),
    ("Final r6",             "r6  = 0x33333333"),
    ("Final r7",             "r7  = 0x22222222"),
    ("Final LR",             "r14 = 0x11111111"),
    ("Final SP",             "r13 = 0x1FFFFFFC"),
    ("Final PC",             "r15 = 0x00008040"),
    ("Final CPSR",           "CPSR = 0x00000000"),
    ("Cycle count",          "cycle=17"),
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