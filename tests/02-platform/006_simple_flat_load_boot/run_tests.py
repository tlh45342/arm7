from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
RUNNER = ROOT / "vm" / "arm7-run" / "bin" / "arm7-run.exe"
GUEST = HERE / "test_simple_flat_load_boot.bin"
DISK = HERE / "disk0-test.img"
SCRIPT = HERE / "build.script"
DUMMY = HERE / "DUMMY.BIN"
BOOT = HERE / "BOOT.BIN"

BOOT_LOAD = 0x00010000
RESULT = 0x00030800
failures = 0

def check(ok: bool, text: str) -> None:
    global failures
    if ok:
        print(f"  PASS {text}")
    else:
        print(f"  FAIL {text}")
        failures += 1

def parse_dump(output: str, addr: int, length: int) -> bytes:
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
        take = min(len(row) - off, end - cur)
        if take <= 0:
            break
        data.extend(row[off:off+take])
        cur += take
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
    print("Running ARM7 SIMPLE-FLAT BOOT payload load validation...")

    flatdisk = find_flatdisk()
    check(flatdisk is not None, "flatdisk found on PATH")
    check(RUNNER.is_file(), "arm7-run exists")
    check(GUEST.is_file(), "guest image exists")
    if flatdisk is None or not RUNNER.is_file() or not GUEST.is_file():
        return 1

    DUMMY.write_bytes(b"DUMMY-FILE-" + bytes(range(27)))

    # 700 bytes deliberately crosses a 512-byte sector boundary.
    payload = bytes(((i * 37 + 11) & 0xFF) for i in range(700))
    BOOT.write_bytes(payload)

    SCRIPT.write_text("\n".join([
        f"create {DISK} 1M",
        f"format {DISK}",
        f"put {DISK} {DUMMY} DUMMY.BIN",
        f"put {DISK} {BOOT} BOOT.BIN",
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
    if fd.returncode != 0:
        return 1

    m = re.search(r"(?m)^BOOT\.BIN\s+(\d+)\s+(\d+)\s+0x([0-9A-Fa-f]+)", fd.stdout)
    expected_lba = int(m.group(1)) if m else None
    expected_len = int(m.group(2)) if m else None
    expected_flags = int(m.group(3), 16) if m else None
    expected_sectors = (len(payload) + 511) // 512

    commands = [
        f"attach disk0 {DISK}",
        f"load {GUEST} 0x00008000",
        "set pc 0x00008000",
        "run",
        f"e 0x{RESULT:08x}-0x{RESULT+47:08x}",
    ]

    # Dump the loaded payload in chunks. arm7-run's examine command emits
    # consecutive 16-byte rows, and this proves the actual guest RAM bytes.
    for off in range(0, len(payload), 128):
        end = min(off + 127, len(payload) - 1)
        commands.append(f"e 0x{BOOT_LOAD+off:08x}-0x{BOOT_LOAD+end:08x}")
    commands += ["regs", "quit", ""]

    p = subprocess.run(
        [str(RUNNER)],
        input="\n".join(commands),
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        cwd=ROOT, timeout=15, check=False
    )
    output = p.stdout
    check(p.returncode == 0, "arm7-run automation completed")

    result = parse_dump(output, RESULT, 48)
    loaded = parse_dump(output, BOOT_LOAD, len(payload))

    found_lba = u32(result, 0)
    found_len = u32(result, 4)
    found_flags = u32(result, 8)
    sectors = u32(result, 12)

    check(found_lba == expected_lba,
          f"guest found BOOT.BIN start LBA {expected_lba}")
    check(found_len == expected_len == len(payload),
          f"guest found BOOT.BIN byte length {len(payload)}")
    check(found_flags == expected_flags and found_flags is not None and (found_flags & 1),
          "guest found BOOT.BIN boot flag")
    check(sectors == expected_sectors == 2,
          "guest calculated two sectors for 700-byte BOOT.BIN")

    check(len(loaded) == len(payload),
          "runner captured complete loaded BOOT region")
    check(loaded == payload,
          "guest RAM BOOT payload exactly matches host BOOT.BIN")

    first_word = int.from_bytes(payload[:4], "little")
    check(u32(result, 16) == first_word,
          "guest recorded correct first payload word")
    check(u32(result, 20) == payload[-1],
          "guest recorded correct final payload byte")
    check(u32(result, 24) == 0x444C4B4F,
          "guest reported BOOT payload load complete")
    check(u32(result, 28) == 0,
          "guest completed without loader failure")

    if expected_lba is not None:
        check(f"[DISK] READ LBA={expected_lba}" in output,
              "runner serviced first BOOT payload sector")
        check(f"[DISK] READ LBA={expected_lba + 1}" in output,
              "runner serviced second BOOT payload sector")
    check("[BKPT]" in output,
          "guest reached BKPT after loading BOOT.BIN")

    if failures:
        print("ARM7 SIMPLE-FLAT BOOT payload load: FAIL")
        print("\n--- arm7-run output ---")
        print(output)
        return 1

    print("ARM7 SIMPLE-FLAT BOOT payload load: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
