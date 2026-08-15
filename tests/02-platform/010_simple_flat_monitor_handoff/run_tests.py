from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
RUNNER = ROOT / "vm" / "arm7-run" / "bin" / "arm7-run.exe"
GUEST = HERE / "test_simple_flat_monitor_handoff.bin"
MONITOR = HERE / "MONITOR.BIN"
DUMMY = HERE / "DUMMY.BIN"
BOOT = HERE / "BOOT.BIN"
DISK = HERE / "disk0-test.img"
SCRIPT = HERE / "build.script"

RESULT = 0x00030800
MONITOR_MARKER = 0x00030C40
MONITOR_LOAD = 0x00020000
CRT_BASE = 0x0A000000
failures = 0

def check(ok: bool, text: str) -> None:
    global failures
    print(f"  {'PASS' if ok else 'FAIL'} {text}")
    if not ok:
        failures += 1

def parse_dump(output: str, addr: int, length: int) -> bytes:
    rows = {}
    row_re = re.compile(r"0x([0-9a-fA-F]{8}):\s*(.*)")
    for line in output.splitlines():
        m = row_re.search(line)
        if not m:
            continue
        a = int(m.group(1), 16)
        vals = re.findall(r"\b[0-9a-fA-F]{2}\b", m.group(2))
        if vals:
            rows[a] = bytes(int(v, 16) for v in vals)

    out = bytearray()
    cur = addr
    end = addr + length
    while cur < end:
        base = cur & ~0xF
        row = rows.get(base)
        if row is None:
            break
        off = cur - base
        n = min(len(row) - off, end - cur)
        if n <= 0:
            break
        out.extend(row[off:off+n])
        cur += n
    return bytes(out)

def u32(b: bytes, off: int):
    return int.from_bytes(b[off:off+4], "little") if len(b) >= off + 4 else None

def find_flatdisk():
    for name in ("flatdisk.exe", "flatdisk"):
        p = shutil.which(name)
        if p:
            return Path(p)
    return None

def main() -> int:
    print("Running ARM7 SIMPLE-FLAT MONITOR handoff validation...")

    flatdisk = find_flatdisk()
    check(flatdisk is not None, "flatdisk found on PATH")
    check(RUNNER.is_file(), "arm7-run exists")
    check(GUEST.is_file(), "loader guest exists")
    check(MONITOR.is_file(), "executable MONITOR.BIN exists")
    if flatdisk is None or not RUNNER.is_file() or not GUEST.is_file() or not MONITOR.is_file():
        return 1

    monitor_bytes = MONITOR.read_bytes()
    check(len(monitor_bytes) == 1148,
          "MONITOR.BIN is 1148 bytes and spans three sectors")

    DUMMY.write_bytes(b"DUMMY-FIRST")
    BOOT.write_bytes(b"BOOT-BEFORE-MONITOR" * 13)

    SCRIPT.write_text("\n".join([
        f"create {DISK} 1M",
        f"format {DISK}",
        f"put {DISK} {DUMMY} DUMMY.BIN",
        f"put {DISK} {BOOT} BOOT.BIN",
        f"put {DISK} {MONITOR} MONITOR.BIN",
        f"list {DISK}",
        "",
    ]), encoding="utf-8")

    fd = subprocess.run(
        [str(flatdisk), "do", str(SCRIPT)],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        cwd=HERE, timeout=15, check=False
    )
    print(fd.stdout, end="")
    check(fd.returncode == 0,
          "flatdisk created executable SIMPLE-FLAT image")
    if fd.returncode != 0:
        return 1

    m = re.search(r"(?m)^MONITOR\.BIN\s+(\d+)\s+(\d+)\s+0x([0-9A-Fa-f]+)", fd.stdout)
    monitor_lba = int(m.group(1)) if m else None
    monitor_len = int(m.group(2)) if m else None

    commands = "\n".join([
        f"attach disk0 {DISK}",
        f"load {GUEST} 0x00008000",
        "set pc 0x00008000",
        "run",
        f"e 0x{RESULT:08x}-0x{RESULT+47:08x}",
        f"e 0x{MONITOR_MARKER:08x}-0x{MONITOR_MARKER+31:08x}",
        f"e 0x{MONITOR_LOAD:08x}-0x{MONITOR_LOAD+31:08x}",
        "show crt",
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
    marker = parse_dump(output, MONITOR_MARKER, 32)
    loaded_head = parse_dump(output, MONITOR_LOAD, 32)

    check(u32(result, 0) == monitor_lba and monitor_lba is not None,
          f"loader found MONITOR.BIN at LBA {monitor_lba}")
    check(u32(result, 4) == monitor_len == 1148,
          "loader obtained 1148-byte MONITOR.BIN length")
    check(u32(result, 12) == 3,
          "loader calculated three MONITOR sectors")
    check(u32(result, 16) == 0x4E4D4B4F,
          "loader reached MONITOR handoff point")
    check(u32(result, 20) == MONITOR_LOAD,
          "loader selected 0x00020000 handoff address")

    check(loaded_head == monitor_bytes[:32],
          "code at 0x00020000 matches disk MONITOR.BIN")

    check(u32(marker, 0) == 0x4E4D4B4F,
          "disk-loaded MONITOR wrote OKMN marker")
    check(u32(marker, 4) == 0x00022007,
          "disk-loaded MONITOR wrote distinctive execution signature")
    check(u32(marker, 8) == MONITOR_LOAD,
          "MONITOR confirms linked/executing at 0x00020000")
    check(u32(marker, 12) == 0x2007CAFE,
          "MONITOR completed marker sequence")

    if monitor_lba is not None:
        check(f"[DISK] READ LBA={monitor_lba}" in output,
              "runner serviced first MONITOR sector")
        check(f"[DISK] READ LBA={monitor_lba + 1}" in output,
              "runner serviced second MONITOR sector")
        check(f"[DISK] READ LBA={monitor_lba + 2}" in output,
              "runner serviced third MONITOR sector")

    check("DISK-LOADED MONITOR OK" in output,
          "disk-loaded MONITOR took visible CRT ownership")

    pc_match = re.search(r"r15\s*=\s*0x([0-9A-Fa-f]+)", output)
    pc = int(pc_match.group(1), 16) if pc_match else None
    check(pc is not None and MONITOR_LOAD <= pc < MONITOR_LOAD + len(monitor_bytes),
          "BKPT stopped execution inside disk-loaded MONITOR region")
    check("[BKPT]" in output,
          "disk-loaded MONITOR reached its BKPT")

    if failures:
        print("ARM7 SIMPLE-FLAT MONITOR handoff: FAIL")
        print("\n--- arm7-run output ---")
        print(output)
        return 1

    print("ARM7 SIMPLE-FLAT MONITOR handoff: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
