import subprocess, re
from pathlib import Path
H=Path(__file__).resolve().parent; R=H.parents[2]
runner=R/"vm/arm7-run/bin/arm7-run.exe"; image=H/"test_stack_push_pop.bin"
print("Running ARM7 PUSH/POP stack validation...")
script=f"load {image} 0x00008000\nset pc 0x00008000\nstep 200\nregs\nquit\n"
p=subprocess.run([str(runner)],input=script,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,cwd=R,timeout=10)
o=p.stdout
checks=[
("[BKPT]" in o,"guest reached BKPT"),
("r11 = 0x4B534B4F" in o,"PUSH/POP restored all values"),
("r13 = 0x001DF000" in o,"SP returned exactly to initial value"),
("r14 = 0xEEEEEEEE" in o,"LR survived PUSH/POP"),
]
bad=0
for ok,msg in checks:
 print(f"  {'PASS' if ok else 'FAIL'} {msg}"); bad += not ok
if bad:
 print("ARM7 PUSH/POP stack: FAIL\n\n--- arm7-run output ---\n"+o); raise SystemExit(1)
print("ARM7 PUSH/POP stack: PASS")
