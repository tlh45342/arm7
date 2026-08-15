  .arch armv7-a
    .arm
    .text
    .global _start
_start:
    mov     r0, #1
    movt    r0, #0x8000          @ r0 = 0x80000001

    mov     r1, #1
    movs    r2, r0, lsl r1       @ r2=0x00000002, C=1
    mrs     r8, cpsr

    mov     r1, #31
    movs    r3, r0, lsr r1       @ r3 = 0x00000001, C=bit30(original)
    mrs     r9, cpsr

    mov     r1, #32
    movs    r4, r0, asr r1       @ r4 = 0xFFFFFFFF (since sign bit=1), C=bit31(original)
    mrs     r10, cpsr

halt:
    .word   0xDEADBEEF
	