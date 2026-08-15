.global _start
_start:
    /* result base = 0x00100000 */
    movw    r6, #0x0000
    movt    r6, #0x0010

    /*
     * Case 1:
     * Insert low 4 bits of r1 into r0 bits 0..3.
     * r0 = 0x12345670
     * r1 = 0x0000000A
     * expected r0 = 0x1234567A
     */
    movw    r0, #0x5670
    movt    r0, #0x1234
    mov     r1, #0x0A
    bfi     r0, r1, #0, #4
    str     r0, [r6, #0]

    /*
     * Case 2:
     * Insert 8 bits into middle field bits 8..15.
     * r2 = 0xDEAD00EF
     * r3 = 0x00000012
     * expected r2 = 0xDEAD12EF
     */
    movw    r2, #0x00EF
    movt    r2, #0xDEAD
    mov     r3, #0x12
    bfi     r2, r3, #8, #8
    str     r2, [r6, #4]

    /*
     * Case 3:
     * Insert 12 bits into bits 12..23.
     * r4 = 0xFFF00000
     * r5 = 0x00000BDF
     * expected r4 = 0xFFBDF000
     */
    movw    r4, #0x0000
    movt    r4, #0xFFF0
    movw    r5, #0x0BDF
    bfi     r4, r5, #12, #12
    str     r4, [r6, #8]

    /*
     * Case 4:
     * Source masking.
     * Insert only low 8 bits of r8 into r7 bits 8..15.
     * r7 = 0xAAAAAAAA
     * r8 = 0x00001234
     * low 8 bits = 0x34
     * expected r7 = 0xAAAA34AA
     */
    movw    r7, #0xAAAA
    movt    r7, #0xAAAA
    movw    r8, #0x1234
    bfi     r7, r8, #8, #8
    str     r7, [r6, #12]

    /*
     * Case 5:
     * Insert into high field bits 28..31.
     * r9  = 0x01234567
     * r10 = 0x0000000C
     * expected r9 = 0xC1234567
     */
    movw    r9, #0x4567
    movt    r9, #0x0123
    mov     r10, #0x0C
    bfi     r9, r10, #28, #4
    str     r9, [r6, #16]

    /*
     * Case 6:
     * BFI should not modify CPSR flags.
     * Set C flag using CMP equal, capture before and after.
     */
    cmp     r0, r0
    mrs     r11, cpsr
    mov     r12, #0
    mov     r14, #0xFF
    bfi     r12, r14, #4, #4
    mrs     r13, cpsr
    str     r11, [r6, #20]
    str     r12, [r6, #24]
    str     r13, [r6, #28]

    // Halt
    bkpt    #0               // halt
	