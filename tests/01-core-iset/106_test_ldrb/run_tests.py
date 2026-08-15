import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_ldrb"

CHECKS = [
    ("Loaded image", "[LOAD] test_ldrb.bin @ 0x00008000"),
    ("PC start", "r15 <= 0x00008000"),

    ("STRB seed A", "[STRB pre-imm] [0x00100100] <= r1 (0x00000041)"),
    ("STRB seed B", "[STRB pre-imm] [0x00100101] <= r1 (0x00000042)"),
    ("STRB seed 0x80", "[STRB pre-imm] [0x00100102] <= r1 (0x00000080)"),
    ("STRB seed 0xFF", "[STRB pre-imm] [0x00100103] <= r1 (0x000000FF)"),

    ("LDRB plain decoded", "[K12] LDRB pre-imm match (key=0x5D0)"),
    ("Plain LDRB result A", "[STR pre-imm] [0x00100000] <= r2 (0x00000041)"),
    ("Plain LDRB base unchanged", "[STR pre-imm] [0x00100004] <= r0 (0x00100100)"),

    ("Offset LDRB result B", "[STR pre-imm] [0x00100008] <= r3 (0x00000042)"),
    ("Offset LDRB base unchanged", "[STR pre-imm] [0x0010000C] <= r0 (0x00100100)"),

    ("Post LDRB #1 decoded", "[K12] LDRB post-imm match (key=0x4D0)"),
    ("Post LDRB #1 result A", "[STR pre-imm] [0x00100010] <= r4 (0x00000041)"),
    ("Post LDRB #1 writeback", "[STR pre-imm] [0x00100014] <= r0 (0x00100101)"),

    ("Post LDRB #2 result B", "[STR pre-imm] [0x00100018] <= r5 (0x00000042)"),
    ("Post LDRB #2 writeback", "[STR pre-imm] [0x0010001C] <= r0 (0x00100102)"),

    ("Zero extend 0x80", "[STR pre-imm] [0x00100020] <= r7 (0x00000080)"),
    ("Post LDRB #3 writeback", "[STR pre-imm] [0x00100024] <= r0 (0x00100103)"),

    ("Zero extend 0xFF", "[STR pre-imm] [0x00100028] <= r8 (0x000000FF)"),
    ("Post LDRB #4 writeback", "[STR pre-imm] [0x0010002C] <= r0 (0x00100104)"),

    ("Register-offset LDRB decoded", "[K12] LDRB reg pre LSL#0 match (key=0x7D0)"),
    ("Register-offset LDRB result", "[STR pre-imm] [0x00100030] <= r9 (0x000000FF)"),
    ("Register-offset base unchanged", "[STR pre-imm] [0x00100034] <= r0 (0x00100100)"),

    ("BKPT", "[BKPT]"),
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