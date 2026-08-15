011 SVC stack-free handler correction

The harness is correct. The failure exposed a kernel-side register-save/restore problem.

Evidence:
- first SVC puts completed
- expected user return address appeared in a general register
- LR/PC wandered into 0x0A... CRT/MMIO space

This version removes PUSH/POP/STM/LDM from the SVC path entirely.

Changed file:
    tests\02-platform\011_svc_console\svc_kernel.s

Retest:
    make -C tests\02-platform\011_svc_console test
