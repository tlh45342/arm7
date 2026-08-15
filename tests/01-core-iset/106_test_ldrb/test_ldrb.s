.global _start

_start:
    /* result base = 0x00100000 */
    movw    r6, #0x0000
    movt    r6, #0x0010

    /* data base = 0x00100100 */
    movw    r0, #0x0100
    movt    r0, #0x0010

    /*
     * Seed bytes:
     * [base+0] = 'A' = 0x41
     * [base+1] = 'B' = 0x42
     * [base+2] = 0x80
     * [base+3] = 0xFF
     */
    mov     r1, #0x41
    strb    r1, [r0]

    mov     r1, #0x42
    strb    r1, [r0, #1]

    mov     r1, #0x80
    strb    r1, [r0, #2]

    mov     r1, #0xFF
    strb    r1, [r0, #3]

    /*
     * Case 1:
     * Plain LDRB [r0]
     * Expect r2 = 0x41
     * Expect r0 unchanged
     */
    ldrb    r2, [r0]
    str     r2, [r6, #0]
    str     r0, [r6, #4]

    /*
     * Case 2:
     * Pre-index immediate LDRB [r0, #1]
     * Expect r3 = 0x42
     * Expect r0 unchanged
     */
    ldrb    r3, [r0, #1]
    str     r3, [r6, #8]
    str     r0, [r6, #12]

    /*
     * Case 3:
     * Post-index immediate LDRB [r0], #1
     * Expect r4 = 0x41
     * Expect r0 = base + 1
     */
    ldrb    r4, [r0], #1
    str     r4, [r6, #16]
    str     r0, [r6, #20]

    /*
     * Case 4:
     * Second post-index immediate LDRB [r0], #1
     * Now r0 points at base + 1.
     * Expect r5 = 0x42
     * Expect r0 = base + 2
     */
    ldrb    r5, [r0], #1
    str     r5, [r6, #24]
    str     r0, [r6, #28]

    /*
     * Case 5:
     * Zero-extension check.
     * r0 currently base + 2.
     * Load 0x80 and 0xFF. LDRB must zero-extend.
     */
    ldrb    r7, [r0], #1
    str     r7, [r6, #32]
    str     r0, [r6, #36]

    ldrb    r8, [r0], #1
    str     r8, [r6, #40]
    str     r0, [r6, #44]

    /*
     * Case 6:
     * Register offset form.
     * Reset r0 to base.
     * r9 = [r0 + r10] where r10 = 3.
     * Expect r9 = 0xFF.
     */
    movw    r0, #0x0100
    movt    r0, #0x0010
    mov     r10, #3
    ldrb    r9, [r0, r10]
    str     r9, [r6, #48]
    str     r0, [r6, #52]

    bkpt    #0x1374
	