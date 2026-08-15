from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
RUNNER = ROOT / "vm" / "arm7-run" / "bin" / "arm7-run.exe"
GUEST = HERE / "test_simple_flat_directory.bin"
DISK = HERE / "disk0-test.img"
SCRIPT = HERE / "build.script"
DUMMY = HERE / "DUMMY.BIN"
BOOT = HERE / "BOOT.BIN"
RESULT = 0x00030800

failures = 0

def check(ok: bool, text: str) -> None:
    global failures
    if ok:
        print(f"  PASS {text}")
    else:
        print(f"  FAIL {text}")
        failures += 1

def parse_dump(output: str, addr: int, length: int = 48) -> bytes:
    """
    Collect a multi-line arm7-run memory dump.

    arm7-run may prefix the first line with "arm7-run> " and emits following
    16-byte rows on separate lines. Parse rows by their actual address and
    assemble the requested byte range rather than assuming everything is on
    the first line.
    """
    rows = {}
    row_re = re.compile(r"0x([0-9a-fA-F]{8}):\s*(.*)")
    for line in output.splitlines():
        m = row_re.search(line)
        if not m:
            continue
        row_addr = int(m.group(1), 16)
        vals = re.findall(r"\b[0-9a-fA-F]{2}\b", m.group(2))
        if vals:
            rows[row_addr] = bytes(int(x, 16) for x in vals)

    data = bytearray()
    cur = addr
    end = addr + length
    while cur < end:
        row_addr = cur & ~0xF
        row = rows.get(row_addr)
        if row is None:
            break
        offset = cur - row_addr
        take = min(len(row) - offset, end - cur)
        if take <= 0:
            break
        data.extend(row[offset:offset + take])
        cur += take
    return bytes(data)

def u32(buf: bytes, off: int):
    if len(buf) < off + 4:
        return None
    return int.from_bytes(buf[off:off+4], "little")

def find_flatdisk():
    for name in ("flatdisk.exe", "flatdisk"):
        p = shutil.which(name)
        if p:
            return Path(p)
    return None

def main() -> int:
    print("Running ARM7 SIMPLE-FLAT directory validation...")

    flatdisk = find_flatdisk()
    check(flatdisk is not None, "flatdisk found on PATH")
    check(RUNNER.is_file(), "arm7-run exists")
    check(GUEST.is_file(), "guest image exists")
    if flatdisk is None or not RUNNER.is_file() or not GUEST.is_file():
        return 1

    # Deliberately make BOOT.BIN larger than one sector and put a different
    # file first. This prevents the guest from succeeding by assuming entry 0.
    DUMMY.write_bytes(b"DUMMY-FILE-" + bytes(range(27)))   # 38 bytes
    BOOT.write_bytes((b"ARM7-BOOT-PAYLOAD-" * 44)[:700])
    boot_len = BOOT.stat().st_size

    SCRIPT.write_text(
        "\n".join([
            f"create {DISK} 1M",
            f"format {DISK}",
            f"put {DISK} {DUMMY} DUMMY.BIN",
            f"put {DISK} {BOOT} BOOT.BIN",
            f"info {DISK}",
            f"list {DISK}",
            "",
        ]),
        encoding="utf-8",
    )

    fd = subprocess.run(
        [str(flatdisk), "do", str(SCRIPT)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=HERE,
        timeout=15,
        check=False,
    )
    print(fd.stdout, end="")
    check(fd.returncode == 0, "flatdisk created SIMPLE-FLAT test image")
    check("BOOT.BIN" in fd.stdout, "flatdisk installed BOOT.BIN")
    if fd.returncode != 0:
        return 1

    # Parse flatdisk's own listing only to establish expected host-side values.
    m = re.search(r"(?m)^BOOT\.BIN\s+(\d+)\s+(\d+)\s+0x([0-9A-Fa-f]+)", fd.stdout)
    expected_lba = int(m.group(1)) if m else None
    expected_len = int(m.group(2)) if m else boot_len
    expected_flags = int(m.group(3), 16) if m else None

    script = "\n".join([
        f"attach disk0 {DISK}",
        f"load {GUEST} 0x00008000",
        "set pc 0x00008000",
        "run",
        f"e 0x{RESULT:08x}-0x{RESULT+47:08x}",
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

    result = parse_dump(output, RESULT)

    check(u32(result, 4) == 0x544C4653,
          "guest recognized SFLT header magic")
    check(u32(result, 8) == 512,
          "guest parsed 512-byte sector size")

    dir_lba = u32(result, 12)
    dir_entries = u32(result, 16)
    dirent_size = u32(result, 20)

    check(dir_lba is not None and dir_lba > 0,
          "guest parsed directory LBA from header")
    check(dir_entries == 32,
          "guest parsed 32 directory entries")
    check(dirent_size == 64,
          "guest parsed 64-byte directory entry size")

    found_lba = u32(result, 24)
    found_len = u32(result, 28)
    found_flags = u32(result, 32)

    check(u32(result, 36) == 0x54424B4F,
          "guest scanned directory and found BOOT.BIN")
    check(expected_lba is not None and found_lba == expected_lba,
          f"guest BOOT.BIN start LBA matches flatdisk ({expected_lba})")
    check(found_len == expected_len == boot_len,
          f"guest BOOT.BIN byte length matches flatdisk ({boot_len})")
    check(expected_flags is not None and found_flags == expected_flags and (found_flags & 1) != 0,
          "guest BOOT.BIN boot flag matches flatdisk")

    fail_code = u32(result, 40)
    check(fail_code == 0,
          "guest completed without parser failure")
    check("[DISK] READ LBA=0" in output,
          "runner serviced SIMPLE-FLAT header read")
    check(dir_lba is not None and f"[DISK] READ LBA={dir_lba}" in output,
          "runner serviced directory-sector read")
    check("[BKPT]" in output,
          "guest reached BKPT after locating BOOT.BIN")

    if failures:
        print("ARM7 SIMPLE-FLAT directory: FAIL")
        print("\n--- arm7-run output ---")
        print(output)
        return 1

    print("ARM7 SIMPLE-FLAT directory: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
