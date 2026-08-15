import subprocess
import os
import sys

VM = "arm7-run.exe"

TEST_NAME = "test_add"

CHECKS = [
    # minimal setup
    ("Loaded image",          "[LOAD] test_add.bin @ 0x00008000"),

    # disassembly anchors (comment + exact address/opcode)
    ("DISASM 8000 MOV",       "00008000:       E3A00000"),
    ("DISASM 8004 ADD",       "00008004:       E2801005"),
    ("DISASM 8008 ADD",       "00008008:       E2812007"),
    ("DISASM 800C MOV",       "0000800C:       E3A0300A"),
    ("DISASM 8010 ADD(reg)",  "00008010:       E0824003"),   # .word in trace; ADD r4,r2,r3
    ("DISASM 8014 MVN(imm)",  "00008014:       E3E05000"),   # .word in trace; MVN r5,#0
    ("DISASM 8018 ADD",       "00008018:       E2856001"),
    ("BKPT word",             "0000801C:       E1212374"),

    # final machine state
    ("Reg r4=22",             "r4  = 0x00000016"),
    ("Reg r6=0",              "r6  = 0x00000000"),
    ("Final PC",              "r15 = 0x0000801C"),
    ("Final CPSR",            "CPSR = 0x00000000"),
    ("Cycle count",           "cycle=8"),
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