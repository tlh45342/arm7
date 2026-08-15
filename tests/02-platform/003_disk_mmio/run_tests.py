import os
import re
import subprocess
import sys

VM = "arm7-run.exe"
TEST = "test_disk_mmio"
LOG = TEST + ".log"
IMAGE = "disk-test.img"

SIGNATURE = b"ARM7DISK-TEST-0001"
FIRST32 = SIGNATURE + b"\x00" * (32 - len(SIGNATURE))

def make_image():
    sector0 = bytearray(512)
    sector0[:len(SIGNATURE)] = SIGNATURE

    # A second recognizable sector makes the image useful for later LBA tests.
    sector1 = bytearray(512)
    marker = b"ARM7DISK-SECTOR-0001"
    sector1[:len(marker)] = marker

    with open(IMAGE, "wb") as f:
        f.write(sector0)
        f.write(sector1)

def main():
    print("Running ARM7 disk MMIO integration validation...")
    make_image()

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

    combined = log + "\n" + (proc.stdout or "")
    ok = True

    def check(cond, label, detail=None):
        nonlocal ok
        if cond:
            print(f"  PASS {label}")
        else:
            print(f"  FAIL {label}")
            if detail:
                print(f"       {detail}")
            ok = False

    check("disk0 attached at 0x0B000000" in combined,
          "disk0 attached through public runner API")
    check("[LOAD] test_disk_mmio.bin @ 0x00008000" in combined,
          "guest image loaded")
    check("[BKPT]" in combined, "guest reached BKPT")

    # Parse three lines:
    # 0x00110000: 16 bytes
    # 0x00110010: 16 bytes
    # 0x00110020: 4 bytes
    lines = {}
    for addr, byte_text in re.findall(
        r"(0x001100(?:00|10|20)):\s*((?:[0-9A-Fa-f]{2}(?:\s+|$))+)",
        combined
    ):
        lines[addr.lower()] = bytes(int(x, 16) for x in byte_text.split())

    if "0x00110000" not in lines or "0x00110010" not in lines:
        check(False, "guest result bytes readable",
              "Could not parse 32-byte guest result block")
    else:
        got = (lines["0x00110000"][:16] +
               lines["0x00110010"][:16])
        check(got == FIRST32,
              "guest copied disk signature from MMIO DATA window",
              f"got={got!r}")

    if "0x00110020" not in lines or len(lines["0x00110020"]) < 4:
        check(False, "final disk STATUS readable")
    else:
        status = int.from_bytes(lines["0x00110020"][:4], "little")
        print(f"       final disk STATUS = 0x{status:08X}")
        check((status & 0x04) == 0, "disk STATUS has no ERR")
        check((status & 0x02) != 0, "disk STATUS has DRQ")

    print("ARM7 disk MMIO integration: " + ("PASS" if ok else "FAIL"))
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(main())
