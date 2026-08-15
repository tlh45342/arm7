from __future__ import annotations

import re
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
RUNNER = ROOT / "vm" / "arm7-run" / "bin" / "arm7-run.exe"
GUEST = HERE / "test_disk_sector_read.bin"
DISK = HERE / "disk0-test.img"

DEST = 0x00030000
RESULT = 0x00030200

failures = 0

def check(ok: bool, text: str) -> None:
    global failures
    if ok:
        print(f"  PASS {text}")
    else:
        print(f"  FAIL {text}")
        failures += 1

def parse_dump(output: str, addr: int) -> bytes:
    """
    arm7-run may prefix monitor output with 'arm7-run> '.
    Search for the address anywhere in the line rather than requiring
    the line to begin with it.
    """
    needle = f"0x{addr:08x}:"
    for line in output.splitlines():
        low = line.lower()
        pos = low.find(needle)
        if pos >= 0:
            payload = line[pos + len(needle):]
            vals = re.findall(r"\b[0-9a-fA-F]{2}\b", payload)
            return bytes(int(x, 16) for x in vals)
    return b""

def main() -> int:
    print("Running ARM7 BIOS-style disk sector read validation...")

    check(RUNNER.is_file(), "arm7-run exists")
    check(GUEST.is_file(), "guest image exists")
    if not RUNNER.is_file() or not GUEST.is_file():
        return 1

    sector = bytearray(512)
    sector[0:4] = b"SFLT"
    sector[4:16] = b"ARM7-SECTOR0"
    DISK.write_bytes(sector + bytes(512 * 7))

    script = "\n".join([
        f"attach disk0 {DISK}",
        f"load {GUEST} 0x00008000",
        "set pc 0x00008000",
        "run",
        f"e 0x{DEST:08x}-0x{DEST+15:08x}",
        f"e 0x{RESULT:08x}-0x{RESULT+31:08x}",
        "regs",
        "quit",
        "",
    ])

    p = subprocess.run(
        [str(RUNNER)],
        input=script,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=ROOT,
        timeout=15,
        check=False,
    )
    output = p.stdout

    check(p.returncode == 0, "arm7-run automation completed")

    dest = parse_dump(output, DEST)
    result = parse_dump(output, RESULT)

    check(len(result) >= 4 and result[0:4] == b"QERD",
          "guest issued BIOS-style disk READ command")

    status = int.from_bytes(result[4:8], "little") if len(result) >= 8 else None
    if status is not None:
        print(f"       final disk STATUS = 0x{status:08X}")
    check(status is not None and (status & 0x04) == 0,
          "disk STATUS has no ERR")
    check(status is not None and (status & 0x02) != 0,
          "disk STATUS has DRQ")

    check(len(dest) >= 4 and dest[0:4] == b"SFLT",
          "LBA 0 copied to guest RAM with SFLT signature")

    first_word = int.from_bytes(result[8:12], "little") if len(result) >= 12 else None
    check(first_word == 0x544C4653,
          "guest observed SFLT first word after 512-byte copy")

    check(len(result) >= 16 and result[12:16] == b"SSAP",
          "guest completed full BIOS-style sector copy")

    check("[DISK] CMD write val=0x81 LBA=0 COUNT=1" in output,
          "runner observed READ command for LBA 0")
    check("[DISK] READ LBA=0" in output,
          "runner serviced LBA 0 read")
    check("[BKPT]" in output,
          "guest reached BKPT after sector copy")

    if failures:
        print("ARM7 BIOS-style disk sector read: FAIL")
        print("\n--- arm7-run output ---")
        print(output)
        return 1

    print("ARM7 BIOS-style disk sector read: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
