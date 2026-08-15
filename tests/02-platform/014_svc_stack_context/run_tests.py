import subprocess
from pathlib import Path

H=Path(__file__).resolve().parent
R=H.parents[2]
run=R/"vm/arm7-run/bin/arm7-run.exe"

print("Running ARM7 stack-based SVC context validation...")

s=f"""load {H/'svc_kernel.bin'} 0x00000000
load {H/'svc_user.bin'} 0x00010000
set pc 0
step 1000
e 0x0003f100-0x0003f10f
regs
quit
"""

p=subprocess.run(
    [str(run)],
    input=s,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    cwd=R,
    timeout=10
)

o=p.stdout.lower()

checks=[
    ("[bkpt]" in o,
     "guest reached BKPT"),

    ("0x0003f100: 4f 4b 53 43" in o,
     "user resumed after stack-based SVC with OKSC marker"),

    ("r0  = 0x0000002a" in o,
     "SVC helper returned 42"),

    ("r3  = 0x33333333" in o and
     "r4  = 0x44444444" in o and
     "r5  = 0x55555555" in o,
     "user register context survived SVC"),

    ("r13 = 0x001ea000" in o,
     "user SP restored after SVC"),

    ("cpsr = 0x60000000" in o,
     "exception return restored pre-SVC CPSR"),
]

bad=0
for ok,msg in checks:
    print(f"  {'PASS' if ok else 'FAIL'} {msg}")
    bad += not ok

if bad:
    print("ARM7 stack-based SVC context: FAIL")
    print("\n--- arm7-run output ---")
    print(p.stdout)
    raise SystemExit(1)

print("ARM7 stack-based SVC context: PASS")
