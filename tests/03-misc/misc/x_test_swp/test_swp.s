    .arch armv7-a
    .arm
    .text
    .global _start

_start:
    // r10 = 0x00100000
    mov   r10, #0
    movt  r10, #0x0010

    // seed memory: [r10] = 0xAABBCCDD
    movw  r1, #0xCCDD
    movt  r1, #0xAABB
    str   r1, [r10]

    // r2 = 0x11223344
    movw  r2, #0x3344
    movt  r2, #0x1122

    swp   r4, r2, [r10]        // r4<=old mem; [r10]<=r2
    ldr   r6, [r10]            // verify [r10] == 0x11223344

    .word 0xDEADBEEF
	