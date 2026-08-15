import os
import re
import subprocess
import sys

VM = "arm7-run.exe"
FLATDISK = "flatdisk.exe"
LOG = "bios-test.log"
IMAGE = "tests/bios-boot-test.img"
SCRIPT = "tests/bios-flatdisk.script"

def run(cmd):
    return subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )

def build_disk():
    for path in (IMAGE, SCRIPT):
        try:
            os.remove(path)
        except FileNotFoundError:
            pass

    script = f"""create {IMAGE} 1M
format {IMAGE}
put {IMAGE} tests/boot-stub.bin BOOT.BIN
info {IMAGE}
list {IMAGE}
"""
    with open(SCRIPT, "w", encoding="utf-8", newline="\n") as f:
        f.write(script)

    p = run([FLATDISK, "do", SCRIPT])
    if p.returncode != 0:
        print("  FAIL building SIMPLE-FLAT BIOS test disk")
        print(p.stdout)
        return False

    print(p.stdout, end="")
    return True

def main():
    print("Running ARM7 BIOS v0.0.3 proven SIMPLE-FLAT boot validation...")

    if not build_disk():
        return 1

    script_text = f"""logfile {LOG}
attach disk0 {IMAGE}
load bios.bin 0x00000000
set pc 0x00000000
run
regs
show crt
quit
"""

    try:
        proc = subprocess.run(
            [VM],
            input=script_text,
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
        print(proc.stdout or "")
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

    check("disk0 attached" in combined,
          "BIOS test disk attached")
    check("[LOAD] bios.bin @ 0x00000000" in combined,
          "BIOS loaded")
    check("[DISK] READ LBA=0" in combined,
          "BIOS read SIMPLE-FLAT header")
    check("[DISK] READ LBA=1" in combined,
          "BIOS read SIMPLE-FLAT directory")
    # BOOT may overwrite the CRT after BIOS prints its intermediate
    # "BOOT.BIN loaded" message, so final CRT state is not reliable proof.
    # In this generated test image BOOT.BIN is at LBA 8; observing that read
    # proves BIOS actually loaded the payload before handoff.
    check("[DISK] READ LBA=8" in combined,
          "BIOS loaded BOOT.BIN payload from disk")
    check("ARM7 BIOS v0.0.3" in combined,
          "BIOS v0.0.3 banner visible")
    check("BOOT HANDOFF OK" in combined,
          "disk-loaded BOOT handoff banner visible")

    m = re.search(r"r15\s*=\s*0x([0-9A-Fa-f]{8})", combined)
    boot_pc_ok = (
        m is not None and
        0x00010000 <= int(m.group(1), 16) < 0x00011000
    )
    check(boot_pc_ok,
          "BIOS transferred control to disk-loaded BOOT region")
    check(boot_pc_ok and "[BKPT]" in combined,
          "disk-loaded BOOT stub reached BKPT")

    if proc.returncode != 0:
        check(False, f"{VM} returned rc={proc.returncode}")

    print("ARM7 BIOS v0.0.3 SIMPLE-FLAT boot: " + ("PASS" if ok else "FAIL"))
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(main())
