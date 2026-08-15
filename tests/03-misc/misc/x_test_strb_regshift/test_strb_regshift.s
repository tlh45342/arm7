    .syntax unified
    .arch armv7-a
    .text
    .global _start
_start:
    // r10 = 0x00100000
    mov   r10, #0
    movt  r10, #0x0010

    mov   r1, #0xA5
    mov   r2, #1

    strb  r1, [r10, r2, lsl #1]       // offset 2, no wb
    strb  r1, [r10, r2, lsl #3]!      // offset 8, writeback -> r10 += 8

    ldrb  r4, [r10]                   // should be 0xA5 (at base+8)
    ldrb  r5, [r10, #-6]              // base+8-6 = base+2 -> 0xA5

    .word 0xDEADBEEF
	