 .arch armv7-a
    .arm
    .text
    .global _start
_start:
    mov     r0, #0xF0
    movt    r0, #0xF0F0          @ r0 = 0xF0F0F0F0

    tst     r0, #0x0F            @ expect Z=1 (low nybble zero)
    mrs     r4, cpsr

    teq     r0, r0               @ r0 ^ r0 -> 0, Z=1
    mrs     r5, cpsr

    mov     r1, #1
    cmp     r1, #1               @ Z=1
    mrs     r6, cpsr
    cmp     r1, #2               @ negative -> N=1
    mrs     r7, cpsr

halt:
    .word   0xDEADBEEF
	