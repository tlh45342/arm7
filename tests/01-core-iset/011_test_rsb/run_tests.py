import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_rsb"

CHECKS = [
    # --- Startup / config ---
    ("debug_enabled",   "[DEBUG] debug_flags set to 0x000003FF"),
    ("load_image",      "[LOAD] test_rsb.bin @ 0x00008000 (120 bytes)"),
    ("pc_start",        "r15 <= 0x00008000"),

    # --- Initial MOVs and first RSB decode ---
    ("mov_r0",          "00008000:       E3A00005        mov r0, #0x5"),
    ("mov_r0_k12",      "[K12] MOV (imm) match (key=0x3A0)"),
    ("mov_r1",          "00008004:       E3A01014        mov r1, #0x14"),
    ("mov_r1_k12",      "[K12] MOV (imm) match (key=0x3A1)"),

    ("rsb_first",       "00008008:       E0602001        .word 0xE0602001"),
    ("rsb_first_k12",   "[K12] RSB match (key=0x060)"),

    # ... (intermediate instructions omitted in this log by design)

    # --- Last RSB (conditional) + decode ---
    ("rsb_last",        "0000806C:       0061E000        .word 0x0061E000"),
    ("rsb_last_k12",    "[K12] RSB match (key=0x060)"),

    # --- BKPT and final summary ---
    ("bkpt_op",         "00008070:       E1212374        bkpt #0x1374"),
    ("bkpt_k12",        "[K12] BKPT match (key=0x127)"),
    ("bkpt_tag",        "[BKPT]"),

    # Final register values (encode all RSB/RSBS tests + flags)
    ("final_r0",        "r0  = 0x00000005"),
    ("final_r1",        "r1  = 0x00000008"),
    ("final_r2",        "r2  = 0x0000007B"),
    ("final_r3",        "r3  = 0x0000007B"),

    ("final_r4",        "r4  = 0x80000000"),  # RSBS (negative case) CPSR snapshot
    ("final_r5",        "r5  = 0x60000000"),  # RSBS equal case CPSR snapshot
    ("final_r6",        "r6  = 0x30000000"),  # RSBS overflow edge CPSR snapshot
    ("final_r7",        "r7  = 0x20000000"),  # RSBS shifted operand CPSR snapshot
    ("final_r8",        "r8  = 0x60000000"),  # RSBS imm equal CPSR snapshot

    ("final_r9",        "r9  = 0x00000000"),  # unused / test scratch
    ("final_r10",       "r10 = 0x7FFFFFFF"),  # big positive result from overflow case
    ("final_r11",       "r11 = 0x0000000B"),  # 11 from rsbs with shift (16 - 5)

    ("final_r13",       "r13 = 0xDEADBEEF"),  # loaded sentinel
    ("final_r14",       "r14 = 0xFFFFFFFD"),  # -3 from final rsbeq
    ("final_r15",       "r15 = 0x00008070"),

    ("final_cpsr",      "CPSR = 0x60000000  cycle=29"),
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