Thought for 12s

Nice! That CMN suite is now behaving exactly as we wanted.

Highlights from your log:

- The first five cases wrote the expected flag bitmasks:
  - +0: `0x40000000` (Z=1)
  - +4: `0x60000000` (Z=1,C=1)
  - +8: `0x90000000` (N=1,V=1)
  - +12:`0x70000000` (Z=1,C=1,V=1)
  - +16:`0x00000000` (all clear)
- The last “RRX with C-in=1” case now stores `0x80000000` at +20 and ends with `CPSR=0x80000000` — N=1, others 0. Perfect ✔️

Notes:

- Your assembler emitted the explicit RRX form (see `E1730064` → CMN with RRX); that’s why the final case now passes.
- Seeing `MOVW/MOVT match` lines at the top is fine — those are just the wide-immediate helpers you used to seed state; the decoder is labeling them correctly.

If you want a quick checklist of other places to sanity-check with the NPC stepper:

- LDR to PC (both literal and register-offset forms).
- POP {...,pc}.
- BL/BX (already NPC-friendly, but worth re-running).
- MRS/MSR/SVC (you already exercised SVC in earlier runs).