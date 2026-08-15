import os
import re
import subprocess
import sys

VM = "arm7-run.exe"
TEST = "test_rtc_mmio"
LOG = TEST + ".log"

def main():
    print("Running ARM7 RTC MMIO integration validation...")

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

    check("[LOAD] test_rtc_mmio.bin @ 0x00008000" in combined,
          "guest image loaded")
    check("[BKPT]" in combined, "guest reached BKPT")

    # Parse the 28 result bytes from the examine-memory output.  The CLI format
    # is the same one used by the keyboard platform test:
    #   0x00100000: xx xx ...
    m = re.search(
        r"0x00100000:\s*((?:[0-9A-Fa-f]{2}\s+){15}[0-9A-Fa-f]{2})"
        r".*?"
        r"0x00100010:\s*((?:[0-9A-Fa-f]{2}\s+){11}[0-9A-Fa-f]{2})",
        combined,
        re.S,
    )

    if not m:
        check(False, "RTC result block readable",
              "Could not parse e 0x00100000-0x0010001B output")
    else:
        raw = bytes(int(x, 16) for x in (m.group(1) + " " + m.group(2)).split())
        vals = [int.from_bytes(raw[i:i+4], "little") for i in range(0, 28, 4)]
        year, month, day, hour, minute, second, status = vals

        print(
            f"       guest RTC = {year:04d}-{month:02d}-{day:02d} "
            f"{hour:02d}:{minute:02d}:{second:02d}, status=0x{status:08X}"
        )

        check(2020 <= year <= 2100, "YEAR sane", f"YEAR={year}")
        check(1 <= month <= 12, "MONTH sane", f"MONTH={month}")
        check(1 <= day <= 31, "DAY sane", f"DAY={day}")
        check(0 <= hour <= 23, "HOUR sane", f"HOUR={hour}")
        check(0 <= minute <= 59, "MINUTE sane", f"MINUTE={minute}")
        check(0 <= second <= 60, "SECOND sane", f"SECOND={second}")
        check(status in (0, 1), "STATUS sane", f"STATUS={status}")

    print("ARM7 RTC MMIO integration: " + ("PASS" if ok else "FAIL"))
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(main())
