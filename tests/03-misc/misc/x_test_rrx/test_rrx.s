   .arch armv7-a
    .arm
    .text
    .global _start
_start:
    mov     r0, #1               @ r0 = 0x00000001

    @ Set C=1 using CMN (0xFFFFFFFF + 1 => carry out set)
    mov     r2, #0xFFFF
    movt    r2, #0xFFFF
    cmn     r2, #1
    mrs     r8, cpsr             @ C=1 now

    movs    r1, r0, rrx          @ r1 = 0x80000000, C becomes 1 (from old bit0)
    mrs     r9, cpsr

halt:
    .word   0xDEADBEEF
	