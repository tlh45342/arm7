import os
import subprocess
import sys

VM = "arm7-run.exe"
TEST_NAME = "test_mrrc2"

def check(label, ok, detail=None):
    if ok:
        print(f"  ✅ {label}")
        return True

    print(f"  ❌ {label}")
    if detail:
        print(f"     {detail}")
    return False

def run_test():
    print(f"Running {TEST_NAME}...")

    script_path = f"{TEST_NAME}.script"
    bin_path = f"{TEST_NAME}.bin"
    log_path = f"{TEST_NAME}.log"

    for path in (script_path, bin_path):
        if not os.path.exists(path):
            print(f"❌ Missing file: {path}")
            return False

    env = os.environ.copy()
    env["PYTHONIOENCODING"] = "utf-8"
    env["PYTHONUTF8"] = "1"

    try:
        with open(script_path, "r", encoding="utf-8") as script:
            proc = subprocess.run(
                [VM],
                stdin=script,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding="utf-8",
                errors="replace",
                env=env,
            )
    except FileNotFoundError:
        print(f"❌ Error: '{VM}' not found in PATH.")
        return False

    if not os.path.exists(log_path):
        print(f"❌ Missing log file: {log_path}")
        if proc.stdout:
            print(proc.stdout)
        return False

    with open(log_path, "r", encoding="utf-8", errors="replace") as f:
        log = f.read()

    passed = True

    passed &= check(
        "Loaded image",
        "[LOAD] test_mrrc2.bin @ 0x00008000" in log,
        "test_mrrc2.bin was not loaded at 0x00008000",
    )

    passed &= check(
        "Pre-state r0 stored",
        "[STR pre-imm] [0x00100000] <= r0 (0xAAAAAAAA)" in log,
        "did not observe the known r0 pre-state",
    )

    passed &= check(
        "Pre-state r1 stored",
        "[STR pre-imm] [0x00100004] <= r1 (0x55555555)" in log,
        "did not observe the known r1 pre-state",
    )

    unknown = "[EXEC] unknown instr" in log
    passed &= check(
        "MRRC2 is not treated as an unknown instruction",
        not unknown,
        "ARM7 currently falls through the unknown-instruction path; "
        "MRRC2 decode/dispatch still needs implementation.",
    )

    # K12 names are diagnostic text, so accept either the expected explicit
    # MRRC2 name or a future coprocessor-group name containing MRRC2.
    mrrc2_match = any(
        "[K12]" in line and "MRRC2" in line and "match" in line
        for line in log.splitlines()
    )
    passed &= check(
        "MRRC2 decoder route",
        mrrc2_match,
        "no K12 MRRC2 decoder match was observed",
    )

    # These are useful once a handler exists. They intentionally do not assert
    # specific CP15 values yet.
    post0 = "[STR pre-imm] [0x00100008] <= r0 " in log
    post1 = "[STR pre-imm] [0x0010000C] <= r1 " in log
    passed &= check(
        "Execution returned from MRRC2 handler",
        post0 and post1,
        "post-MRRC2 register stores were not both observed",
    )

    passed &= check("BKPT reached", "[BKPT]" in log)

    if proc.returncode != 0:
        passed = False
        print(f"  ❌ {VM} returned rc={proc.returncode}")

    print(f"{TEST_NAME}: {'✅ passed' if passed else '❌ failed'}")
    return passed

if __name__ == "__main__":
    sys.exit(0 if run_test() else 1)
