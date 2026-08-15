   .text
        .global _start
_start:
        // base @ 0x00100000
        mov     r10, #0x00
        movt    r10, #0x0010

        // r2 = 0xBEEF, STRH [r10,#2]!
        mov     r2, #0xBEEF & 0xFFFF
        strh    r2, [r10, #2]!

        // LDRH -> r3 should be 0x0000BEEF
        ldrh    r3, [r10]

        // Write a signed byte 0x80 at [r10,#4], then LDRSB -> r4 = 0xFFFFFF80
        mov     r1, #0x80
        strb    r1, [r10, #4]
        ldrsb   r4, [r10, #4]

        // Write signed halfword 0x8001 at [r10,#6], then LDRSH -> r5 = 0xFFFF8001
        mov     r1, #0x01
        movt    r1, #0x80
        strh    r1, [r10, #6]
        ldrsh   r5, [r10, #6]

        .word   0xDEADBEEF
		