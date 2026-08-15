011 bounded diagnostic harness

Changed file:
    tests\02-platform\011_svc_console\run_tests.py

The original test used 'run', which can spin forever if the SVC
entry/return path is wrong.

This version uses:
    step 4000

and always captures:
    CRT
    0x0003F000 result/cursor state
    registers

No kernel or user assembly changes are made.

Run:
    make -C tests\02-platform\011_svc_console test

If it fails, send the complete output after:
    --- bounded arm7-run snapshot ---

That snapshot should tell us exactly whether the fault is:
    SVC entry,
    vector dispatch,
    handler execution,
    MOVS pc,lr exception return,
    or repeated-call state.
