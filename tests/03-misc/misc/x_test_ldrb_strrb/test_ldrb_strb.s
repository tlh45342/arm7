   .arch armv7-a
    .arm
    .data
wordval:
    .word   0xAABBCCDD           @ bytes @+0..+3 = DD CC BB AA
    .text
    .global _start
_start:
    ldr     r0, =wordval
    mov     r1, #0x11
    strb    r1, [r0, #2]         @ overwrite the 'BB' byte -> new word = 0xAA11CCDD

    ldr     r2, [r0]             @ expect r2 = 0xAA11CCDD

    ldrb    r3, [r0, #1]         @ expect r3 = 0xCC

halt:
    .word   0xDEADBEEF
    .ltorg
	