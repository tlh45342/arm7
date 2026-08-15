import subprocess
import os
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_svc"

CHECKS = [
    # vector copy setup + loop
    ("ldr pc, [pc, #24]        ; vector entry 0 -> 0x8040", "00008000:       E59FF018"),
    ("ldr r0, [pc, #88]        ; r0 = &vector_table (0x8000)", "00008040:       E59F0058"),
    ("mov r1, #0               ; dst = 0x00000000",          "00008044:       E3A01000"),
    ("mov r2, #0x40            ; bytes to copy",              "00008048:       E3A02040"),
    ("ldr r3, [r0], #4         ; copy loop: load",            "0000804C:       E4903004"),
    ("str r3, [r1], #4         ; copy loop: store",           "00008050:       E4813004"),
    ("subs r2, r2, #4          ; decrement counter",          "00008054:       E2522004"),
    ("bne 0x0000804C           ; loop if r2 != 0",            "00008058:       1AFFFFFB"),

    # barriers
    ("dsb sy                   ; ensure table visible",       "0000805C:       F57FF04F"),
    ("isb sy                   ; sync pipeline",               "00008060:       F57FF06F"),

    # clear marker + SVC
    ("mov r0, #0x100000        ; MARK_ADDR",                  "00008064:       E3A00601"),
    ("mov r1, #0               ; clear",                      "00008068:       E3A01000"),
    ("str r1, [r0]             ; *MARK_ADDR = 0",             "0000806C:       E5801000"),
    ("svc #0x42                ; trigger SVC",                "00008070:       EF000042"),

    # vector indirection to SVC handler
    ("ldr pc, [pc, #24]        ; vector SVC slot -> 0x8088",  "00000008:       E59FF018"),

    # SVC handler
    ("sub r0, lr, #4           ; address of SVC instr",       "00008088:       E24E0004"),
    ("ldr r1, [r0]             ; fetch SVC opcode",           "0000808C:       E5901000"),
    ("mov r2, #0x100000        ; MARK_ADDR",                  "00008090:       E3A02601"),
    ("ldr r3, [pc, #12]        ; r3 = 0xDEADBEEF",            "00008094:       E59F300C"),
    ("str r3, [r2]             ; *MARK_ADDR = DEADBEEF",      "00008098:       E5823000"),
    ("movs pc, lr              ; return from SVC",            "0000809C:       E1B0F00E"),

    # resumed main path
    ("mov r0, #0x100000        ; MARK_ADDR",                  "00008074:       E3A00601"),
    ("ldr r1, [pc, #36]        ; r1 = 0x12345678",            "00008078:       E59F1024"),
    ("str r1, [r0]             ; *MARK_ADDR = 12345678",      "0000807C:       E5801000"),

    ("BKPT", "00008080:       E121237"),
    ("Cycle count=85",          "cycle=85"),
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