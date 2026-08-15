.global _start
_start:
    /* result base = 0x00100000 */
    movw    r6, #0x0000
    movt    r6, #0x0010

    /*
     * Case 1: C=1, value=2
     * 0x00000002 RRX with C=1 -> 0x80000001, C=0, N=1
     */
    mvn     r8, #0
    adds    r8, r8, #1        @ C=1
    mov     r3, #2
    movs    r4, r3, rrx
    mrs     r10, cpsr
    str     r4,  [r6, #0]
    str     r10, [r6, #4]

    /*
     * Case 2: C=0, value=2
     * 0x00000002 RRX with C=0 -> 0x00000001, C=0, N=0, Z=0
     */
    mov     r5, #1
    mov     r7, #0
    cmp     r7, r5            @ C=0
    movs    r7, r3, rrx
    mrs     r10, cpsr
    str     r7,  [r6, #8]
    str     r10, [r6, #12]

    /*
     * Case 3: C=0, value=1
     * 0x00000001 RRX with C=0 -> 0x00000000, C=1, Z=1
     */
    mov     r5, #1
    mov     r7, #0
    cmp     r7, r5            @ C=0
    mov     r3, #1
    movs    r8, r3, rrx
    mrs     r10, cpsr
    str     r8,  [r6, #16]
    str     r10, [r6, #20]

    /*
     * Case 4: C=1, value=1
     * 0x00000001 RRX with C=1 -> 0x80000000, C=1, N=1
     */
    mvn     r9, #0
    adds    r9, r9, #1        @ C=1
    mov     r3, #1
    movs    r9, r3, rrx
    mrs     r10, cpsr
    str     r9,  [r6, #24]
    str     r10, [r6, #28]

    /*
     * Case 5: plain MOV RRX should not update CPSR.
     * Set CPSR to C=1, then MOV without S.
     * Result changes, CPSR should remain C=1.
     */
    mvn     r11, #0
    adds    r11, r11, #1      @ C=1
    mrs     r12, cpsr
    mov     r0, #2
    mov     r1, r0, rrx       @ no S: flags unchanged
    mrs     r13, cpsr
    str     r1,  [r6, #32]
    str     r12, [r6, #36]
    str     r13, [r6, #40]

    bkpt    #0x1374
	