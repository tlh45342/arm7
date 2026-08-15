# 138 — SMC

Status: **PLANNED / NOT YET VERIFIED**

This directory reserves the canonical ARM7/A32 core-test slot for:

```text
138  SMC
```

No implementation or test result is claimed by this placeholder.

When this instruction is brought online, the preferred contents are:

```text
Makefile
linker.ld
run_tests.py
test_smc.s
test_smc.script
README.md
```

The test should prefer behavioral checks over matching incidental debug text.
