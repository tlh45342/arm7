import subprocess
from pathlib import Path

H=Path(__file__).resolve().parent
R=H.parents[2]
runner=R/"vm/arm7-run/bin/arm7-run.exe"
image=H/"test_stack_nested_calls.bin"

print("Running ARM7 nested BL/stack validation...")

s=f"""load {image} 0x00008000
set pc 0x00008000
step 300
regs
quit
"""

p=subprocess.run(
    [str(runner)],
    input=s,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    cwd=R,
    timeout=10
)
o=p.stdout

checks=[
    ("[BKPT]" in o, "guest reached BKPT"),
    ("r11 = 0x434E4B4F" in o, "three-level nested call returned correctly"),
    ("r0  = 0x00000008" in o, "nested helpers produced expected result 8"),
    ("r13 = 0x001DD000" in o, "nested frames restored SP"),
]

bad=0
for ok,msg in checks:
    print(f"  {'PASS' if ok else 'FAIL'} {msg}")
    bad += not ok

if bad:
    print("ARM7 nested BL/stack: FAIL")
    print("\n--- arm7-run output ---")
    print(o)
    raise SystemExit(1)

print("ARM7 nested BL/stack: PASS")
