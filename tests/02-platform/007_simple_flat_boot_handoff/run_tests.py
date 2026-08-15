from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
RUNNER = ROOT / "vm" / "arm7-run" / "bin" / "arm7-run.exe"
GUEST = HERE / "test_simple_flat_boot_handoff.bin"
BOOT = HERE / "BOOT.BIN"
DUMMY = HERE / "DUMMY.BIN"
DISK = HERE / "disk0-test.img"
SCRIPT = HERE / "build.script"

RESULT = 0x00030800
BOOT_MARKER = 0x00030C00
BOOT_LOAD = 0x00010000
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
    while cur < addr + length:
        base = cur & ~0xF
        row = rows.get(base)
        if row is None:
            break
        off = cur - base
        n = min(len(row) - off, addr + length - cur)
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
    print("Running ARM7 SIMPLE-FLAT BOOT handoff validation...")

    flatdisk = find_flatdisk()
    check(flatdisk is not None, "flatdisk found on PATH")
    check(RUNNER.is_file(), "arm7-run exists")
    check(GUEST.is_file(), "loader guest exists")
    check(BOOT.is_file(), "executable BOOT.BIN exists")
    if flatdisk is None or not RUNNER.is_file() or not GUEST.is_file() or not BOOT.is_file():
        return 1

    boot_bytes = BOOT.read_bytes()
    check(len(boot_bytes) == 700, "BOOT.BIN is 700 bytes and spans two sectors")

    DUMMY.write_bytes(b"DUMMY-BEFORE-BOOT")
    SCRIPT.write_text("\n".join([
        f"create {DISK} 1M",
        f"format {DISK}",
        f"put {DISK} {DUMMY} DUMMY.BIN",
        f"put {DISK} {BOOT} BOOT.BIN",
        f"list {DISK}",
        "",
    ]), encoding="utf-8")

    fd = subprocess.run(
        [str(flatdisk), "do", str(SCRIPT)],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        cwd=HERE, timeout=15, check=False
    )
    print(fd.stdout, end="")
    check(fd.returncode == 0, "flatdisk created executable SIMPLE-FLAT image")
    if fd.returncode != 0:
        return 1

    m = re.search(r"(?m)^BOOT\.BIN\s+(\d+)\s+(\d+)\s+0x([0-9A-Fa-f]+)", fd.stdout)
    boot_lba = int(m.group(1)) if m else None
    boot_len = int(m.group(2)) if m else None

    commands = "\n".join([
        f"attach disk0 {DISK}",
        f"load {GUEST} 0x00008000",
        "set pc 0x00008000",
        "run",
        f"e 0x{RESULT:08x}-0x{RESULT+47:08x}",
        f"e 0x{BOOT_MARKER:08x}-0x{BOOT_MARKER+31:08x}",
        f"e 0x{BOOT_LOAD:08x}-0x{BOOT_LOAD+31:08x}",
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
    marker = parse_dump(output, BOOT_MARKER, 32)
    loaded_head = parse_dump(output, BOOT_LOAD, 32)

    check(u32(result, 0) == boot_lba and boot_lba is not None,
          f"loader found BOOT.BIN at LBA {boot_lba}")
    check(u32(result, 4) == boot_len == 700,
          "loader obtained 700-byte BOOT.BIN length")
    check(u32(result, 12) == 2,
          "loader calculated two payload sectors")
    check(u32(result, 16) == 0x444C4B4F,
          "loader reached handoff point")
    check(u32(result, 20) == BOOT_LOAD,
          "loader selected 0x00010000 handoff address")

    check(loaded_head == boot_bytes[:32],
          "code at 0x00010000 matches disk BOOT.BIN")

    check(u32(marker, 0) == 0x54424B4F,
          "disk-loaded BOOT code wrote OKBT marker")
    check(u32(marker, 4) == 0x0001B007,
          "disk-loaded BOOT wrote distinctive execution signature")
    check(u32(marker, 8) == BOOT_LOAD,
          "BOOT code confirms it is linked/executing at 0x00010000")
    check(u32(marker, 12) == 0xB007CAFE,
          "BOOT code completed marker sequence")

    if boot_lba is not None:
        check(f"[DISK] READ LBA={boot_lba}" in output,
              "runner serviced first BOOT sector")
        check(f"[DISK] READ LBA={boot_lba + 1}" in output,
              "runner serviced second BOOT sector")

    # After BKPT in the BOOT stub, PC should be inside the loaded BOOT region,
    # not back in the loader at 0x8000.
    pc_match = re.search(r"r15\s*=\s*0x([0-9A-Fa-f]+)", output)
    pc = int(pc_match.group(1), 16) if pc_match else None
    check(pc is not None and BOOT_LOAD <= pc < BOOT_LOAD + len(boot_bytes),
          "BKPT stopped execution inside disk-loaded BOOT region")
    check("[BKPT]" in output,
          "disk-loaded BOOT reached its BKPT")

    if failures:
        print("ARM7 SIMPLE-FLAT BOOT handoff: FAIL")
        print("\n--- arm7-run output ---")
        print(output)
        return 1

    print("ARM7 SIMPLE-FLAT BOOT handoff: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
