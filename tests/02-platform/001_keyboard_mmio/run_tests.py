import os
import subprocess
import sys

VM = "arm7-run.exe"
TEST = "test_keyboard_mmio"
LOG = TEST + ".log"

CHECKS = [
    ("key queued through CLI", "[KEY] queued 0x41 ('A') pending=1"),
    ("guest image loaded", "[LOAD] test_keyboard_mmio.bin @ 0x00008000"),
    ("guest reached BKPT", "[BKPT]"),
    ("guest stored ASCII A", "0x00100000: 41 00 00 00"),
    ("FIFO drained after DATA read", "41 00 00 00 00 00 00 00"),
]

def main():
    print("Running ARM7 keyboard MMIO integration validation...")

    if os.path.exists(LOG):
        os.remove(LOG)

    try:
        with open(TEST + ".script", "r", encoding="utf-8") as script:
            proc = subprocess.run(
                [VM],
                stdin=script,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
    except FileNotFoundError:
        print(f"  FAIL {VM} not found in PATH")
        return 1

    if not os.path.exists(LOG):
        print(f"  FAIL missing {LOG}")
        if proc.stdout:
            print(proc.stdout)
        return 1

    with open(LOG, "r", encoding="utf-8", errors="replace") as f:
        log = f.read()

    # `e` output may be stdout-only depending on logging path, so check both.
    combined = log + "\n" + (proc.stdout or "")

    ok = True
    for label, needle in CHECKS:
        if needle in combined:
            print(f"  PASS {label}")
        else:
            print(f"  FAIL {label}")
            print(f"       Missing: {needle}")
            ok = False

    print("ARM7 keyboard MMIO integration: " + ("PASS" if ok else "FAIL"))
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(main())
