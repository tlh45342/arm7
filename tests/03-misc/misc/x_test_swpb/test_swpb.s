 .arch armv7-a
    .arm
    .text
    .global _start
_start:
    // r10 = 0x00100000
    mov   r10, #0
    movt  r10, #0x0010

    // seed byte: [r10] = 0x5A
    mov   r1, #0x5A
    strb  r1, [r10]

    // r2 = 0x000000C3 (low byte used)
    mov   r2, #0xC3

    swpb  r4, r2, [r10]       // r4<=old byte; [r10]<=0xC3
    ldrb  r6, [r10]

    .word 0xDEADBEEF
	