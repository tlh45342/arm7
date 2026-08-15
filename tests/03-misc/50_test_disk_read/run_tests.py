import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_disk_read"

CHECKS = [
    ("Debug enabled", "[DEBUG] debug_flags set to 0xFFFFFFFF"),

    # Device attach (you have two lines; either is fine—keep one or both)
    ("Disk attached mmio", "[DISK] disk0 attached at 0x0B000000"),
    # ("Disk attached mgr",  "[DISK] disk0 attached: floppy.img"),

    # Command + read
    ("Set LBA=19",          "[MOV imm] r1 = 0x00000013"),
    ("Write LBA reg",       "mem[0x0B000004] <= r1 (0x00000013)"),
    ("Set DST=0x10000",     "mem[0x0B000008] <= r4 (0x00010000)"),
    ("Kick read",           "[DISK] READ LBA=19 -> dst=0x00010000"),
    ("Read complete",       "[DISK] READ complete (512 bytes) first8=48 45 4c 4c 4f 20 20 20"),

    # Using the data
    ("Load first byte",     "[LDRB reg] r2 = mem8[0x00010000] => 0x48"),
    ("UART wrote 'H'",      "[STRB pre-inc imm] mem[0x09000000] <= r2 (0x48)"),

    # Optional: spot-check the dump line (note lowercase hex in the dump)
    ("Dump shows HELLO.TXT","0x00010000: 48 45 4c 4c 4f 20 20 20 54 58 54 20"),

    # Halt
    ("[HALT] DEADBEEF",     "[HALT] DEADBEEF"),
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