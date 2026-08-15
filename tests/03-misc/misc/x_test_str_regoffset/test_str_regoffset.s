.syntax unified
    .arch armv7-a
    .text
    .align 2
    .global _start
_start:
    // r10 = 0x00100000
    mov   r10, #0
    movt  r10, #0x0010

    // r1 = 0x11223344
    movw  r1, #0x3344
    movt  r1, #0x1122

    mov   r2, #3
    str   r1, [r10, r2, lsl #2]!      // pre-index, writeback (+12)

    mov   r3, #1
    str   r1, [r10, -r3, lsl #2]      // pre-index, no writeback (-4)

    ldr   r4, [r10]                   // should read 0x11223344 from base+12

    .word 0xDEADBEEF
	