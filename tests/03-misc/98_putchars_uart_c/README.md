
This is a minimal setup.

This consists of a c file.
The makefile has the parameters to set the hardcoded expecte starting address of 0x8000
the C file uses INLINE ASM to set a STACK and to use our SPECIAL DEADBEEF code for halting the VM

This is in comparison to using a start.s file with a something.c file that has a "main" that start.s branches to.
And then depending on a .ld file "link" everything.