    .arch armv7-a
    .arm
    .text
    .global _start
_start:
    dmb     sy
    dsb     sy
    isb     sy
halt:
    .word   0xDEADBEEF
	