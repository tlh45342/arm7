from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
RUNNER = ROOT / "vm" / "arm7-run" / "bin" / "arm7-run.exe"
GUEST = HERE / "test_simple_flat_find_monitor.bin"
DISK = HERE / "disk0-test.img"
SCRIPT = HERE / "build.script"
DUMMY = HERE / "DUMMY.BIN"
BOOT = HERE / "BOOT.BIN"
MONITOR = HERE / "MONITOR.BIN"
RESULT = 0x00030800

failures = 0

def check(ok: bool, text: str) -> None:
    global failures
    print(f"  {'PASS' if ok else 'FAIL'} {text}")
    if not ok:
        failures += 1

def parse_dump(output: str, addr: int, length: int = 48) -> bytes:
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
        off = cur - row_addr
        n = min(len(row) - off, end - cur)
        if n <= 0:
            break
        data.extend(row[off:off+n])
        cur += n
    return bytes(data)

def u32(buf: bytes, off: int):
    return int.from_bytes(buf[off:off+4], "little") if len(buf) >= off + 4 else None

def find_flatdisk():
    for name in ("flatdisk.exe", "flatdisk"):
        p = shutil.which(name)
        if p:
            return Path(p)
    return None

def main() -> int:
    print("Running ARM7 SIMPLE-FLAT MONITOR lookup validation...")

    flatdisk = find_flatdisk()
    check(flatdisk is not None, "flatdisk found on PATH")
    check(RUNNER.is_file(), "arm7-run exists")
    check(GUEST.is_file(), "guest image exists")
    if flatdisk is None or not RUNNER.is_file() or not GUEST.is_file():
        return 1

    # Multiple preceding entries force a real directory scan.
    DUMMY.write_bytes(b"DUMMY-FIRST")
    BOOT.write_bytes(b"BOOT-PAYLOAD-" * 19)
    MONITOR.write_bytes((b"ARM7-MONITOR-PAYLOAD-" * 71)[:1148])
    monitor_len = MONITOR.stat().st_size

    SCRIPT.write_text("\n".join([
        f"create {DISK} 1M",
        f"format {DISK}",
        f"put {DISK} {DUMMY} DUMMY.BIN",
        f"put {DISK} {BOOT} BOOT.BIN",
        f"put {DISK} {MONITOR} MONITOR.BIN",
        f"info {DISK}",
        f"list {DISK}",
        "",
    ]), encoding="utf-8")

    fd = subprocess.run(
        [str(flatdisk), "do", str(SCRIPT)],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        cwd=HERE, timeout=15, check=False
    )
    print(fd.stdout, end="")
    check(fd.returncode == 0, "flatdisk created SIMPLE-FLAT test image")
    check("MONITOR.BIN" in fd.stdout, "flatdisk installed MONITOR.BIN")
    if fd.returncode != 0:
        return 1

    m = re.search(r"(?m)^MONITOR\.BIN\s+(\d+)\s+(\d+)\s+0x([0-9A-Fa-f]+)", fd.stdout)
    expected_lba = int(m.group(1)) if m else None
    expected_len = int(m.group(2)) if m else monitor_len
    expected_flags = int(m.group(3), 16) if m else None

    commands = "\n".join([
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
        [str(RUNNER)], input=commands, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        cwd=ROOT, timeout=15, check=False
    )
    output = p.stdout
    check(p.returncode == 0, "arm7-run automation completed")

    result = parse_dump(output, RESULT, 48)

    check(u32(result, 0) == 0x544C4653,
          "guest recognized SFLT header")
    dir_lba = u32(result, 4)
    check(dir_lba is not None and dir_lba > 0,
          "guest parsed directory LBA")
    check(u32(result, 8) == 32,
          "guest parsed 32 directory entries")
    check(u32(result, 12) == 64,
          "guest parsed 64-byte directory entry size")

    found_lba = u32(result, 16)
    found_len = u32(result, 20)
    found_flags = u32(result, 24)

    check(u32(result, 28) == 0x4E4D4B4F,
          "guest scanned directory and found MONITOR.BIN")
    check(expected_lba is not None and found_lba == expected_lba,
          f"guest MONITOR.BIN start LBA matches flatdisk ({expected_lba})")
    check(found_len == expected_len == monitor_len,
          f"guest MONITOR.BIN byte length matches flatdisk ({monitor_len})")
    check(expected_flags is not None and found_flags == expected_flags,
          "guest MONITOR.BIN flags match flatdisk")
    check(u32(result, 32) == 0,
          "guest completed without lookup failure")

    check("[DISK] READ LBA=0" in output,
          "runner serviced header read")
    check(dir_lba is not None and f"[DISK] READ LBA={dir_lba}" in output,
          "runner serviced directory read")
    check("[BKPT]" in output,
          "guest reached BKPT after locating MONITOR.BIN")

    if failures:
        print("ARM7 SIMPLE-FLAT MONITOR lookup: FAIL")
        print("\n--- arm7-run output ---")
        print(output)
        return 1

    print("ARM7 SIMPLE-FLAT MONITOR lookup: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
