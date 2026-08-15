        .text
 .global _start
_start:
        // r3 = 0x00100000 (base)
        movw    r3, #0x0000
        movt    r3, #0x0010

        // r2 = 0x11111111, STR pre-index + write-back at [r3,#8]!
        movw    r2, #0x1111
        movt    r2, #0x1111
        str     r2, [r3, #8]!

        // LDR from updated base -> r4 = 0x11111111
        ldr     r4, [r3]

        // r2 = 0x22222222, STR post-index at [r3],#12
        movw    r2, #0x2222
        movt    r2, #0x2222
        str     r2, [r3], #12

        // Read back the post-indexed write target via negative pre-offset
        ldr     r5, [r3, #-12]

        // r6 = 0xCAFEBABE
        movw    r6, #0xBABE
        movt    r6, #0xCAFE

        // Pre-index (no write-back) then pre-index + write-back LDR
        str     r6, [r3, #10]
        ldr     r7, [r3, #10]!

        // Post-index LDR; should still read r6 value before increment
        ldr     r8, [r3], #4

        .word   0xDEADBEEF
	