import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_putchars"

CHECKS = [
    # Probe shows the source buffer contents
    ("0x0000803c: 58 59 50 51 0a 00", "0x0000803c: 58 59 50 51 0a 00"),

    # Function entry: BL to putchars(), LR set
    ("[BL] target=0x00008010 LR=0x0000800C", "[BL] target=0x00008010 LR=0x0000800C"),

    # First byte 'X'
    ("[LDRB post-imm] r0 = mem8[0x0000803C] => 0x58; r2 += 0x1",
     "[LDRB post-imm] r0 = mem8[0x0000803C] => 0x58; r2 += 0x1"),

    # Prove we actually call the putchar stub
    ("[BL] target=0x00008038 LR=0x0000802C", "[BL] target=0x00008038 LR=0x0000802C"),
    ("[BX] r14 => 0x0000802C", "[BX] r14 => 0x0000802C"),

    # A middle byte ('P') to show the loop continues
    ("[LDRB post-imm] r0 = mem8[0x0000803E] => 0x50; r2 += 0x1",
     "[LDRB post-imm] r0 = mem8[0x0000803E] => 0x50; r2 += 0x1"),

    # Newline byte
    ("[LDRB post-imm] r0 = mem8[0x00008040] => 0x0A; r2 += 0x1",
     "[LDRB post-imm] r0 = mem8[0x00008040] => 0x0A; r2 += 0x1"),

    # NUL terminator leads to exit branch
    ("[LDRB post-imm] r0 = mem8[0x00008041] => 0x00; r2 += 0x1",
     "[LDRB post-imm] r0 = mem8[0x00008041] => 0x00; r2 += 0x1"),
    ("[B] target=0x00008030", "[B] target=0x00008030"),

    # Epilogue restores and returns to caller
    ("[LDM] r11 <= mem[0x1FFFFFF4] => 0x00000000",
     "[LDM] r11 <= mem[0x1FFFFFF4] => 0x00000000"),
    ("[LDM] r14 <= mem[0x1FFFFFF8] => 0x0000800C",
     "[LDM] r14 <= mem[0x1FFFFFF8] => 0x0000800C"),
    ("[BX] r14 => 0x0000800C", "[BX] r14 => 0x0000800C"),

    # Sentinel halt
    ("[HALT] DEADBEEF", "[HALT] DEADBEEF"),
]

def run_test():
    print(f"Running {TEST_NAME}.script...")

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