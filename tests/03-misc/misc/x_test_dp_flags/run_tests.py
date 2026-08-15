import subprocess
import os
import sys

VM = "arm-vm.exe"

TEST_NAME = "test_dp_flags"

CHECKS = [
    # setup / config
    ("Debug enabled",   "[DEBUG] debug_flags set to 0x000003FF"),
    ("Loaded image",    "[LOAD] test_dp_flags.bin @ 0x00008000"),
    ("PC start",        "r15 <= 0x00008000"),

    # Case A build + add (use disasm and opcode traces)
    ("A movw",          "00008000:       E30F0FFF        movw r0, #0xFFFF"),
    ("A movt",          "00008004:       E3470FFF        movt r0, #0x7FFF"),
    ("A addS opcode",   "[TRACE] PC=0x0000800C Instr=0xE0902001"),
    ("A mrs opcode",    "[TRACE] PC=0x00008010 Instr=0xE10FA000"),

    # Case B build + add
    ("B movw",          "00008014:       E30F0FFF        movw r0, #0xFFFF"),
    ("B movt",          "00008018:       E34F0FFF        movt r0, #0xFFFF"),
    ("B addS opcode",   "[TRACE] PC=0x00008020 Instr=0xE0903001"),
    ("B mrs opcode",    "[TRACE] PC=0x00008024 Instr=0xE10FB000"),

    # ADC consumes carry from Case B
    ("ADC opcode",      "[TRACE] PC=0x0000802C Instr=0xE2A06000"),
    ("ADC mrs opcode",  "[TRACE] PC=0x00008030 Instr=0xE10FC000"),

    # graceful stop (include if your runner prints it; otherwise omit)
    # ("DEADBEEF trap",   "[HALT] DEADBEEF."),

    # results / flags
    ("A r2",            "r2  = 0x80000000"),
    ("A flags",         "r10 = 0x90000000"),
    ("B r3",            "r3  = 0x00000000"),
    ("B flags",         "r11 = 0x60000000"),
    ("ADC r6",          "r6  = 0x00000001"),
    ("ADC flags",       "r12 = 0x60000000"),

    # tail state
    ("Final PC",        "r15 = 0x00008034"),
    ("Final CPSR",      "CPSR = 0x60000000"),
    ("Cycle count",     "cycle=14"),
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