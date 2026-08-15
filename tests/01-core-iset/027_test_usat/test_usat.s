.global _start
_start:
    /* result base = 0x00100000 */
    movw    r6, #0x0000
    movt    r6, #0x0010

    /*
     * Case 1:
     * negative input -> 0
     * Q should be set.
     */
    msr     CPSR_f, #0
    mvn     r1, #9              @ -10
    usat    r0, #8, r1
    mrs     r10, cpsr
    str     r0,  [r6, #0]
    str     r10, [r6, #4]

    /*
     * Case 2:
     * in range 100 -> 100
     * Q should stay clear.
     */
    msr     CPSR_f, #0
    mov     r1, #100
    usat    r2, #8, r1
    mrs     r10, cpsr
    str     r2,  [r6, #8]
    str     r10, [r6, #12]

    /*
     * Case 3:
     * upper edge 255 -> 255
     * Q should stay clear.
     */
    msr     CPSR_f, #0
    mov     r1, #255
    usat    r3, #8, r1
    mrs     r10, cpsr
    str     r3,  [r6, #16]
    str     r10, [r6, #20]

    /*
     * Case 4:
     * above range 300 -> 255
     * Q should be set.
     */
    msr     CPSR_f, #0
    movw    r1, #300
    usat    r4, #8, r1
    mrs     r10, cpsr
    str     r4,  [r6, #24]
    str     r10, [r6, #28]

    /*
     * Case 5:
     * shifted input: 200 << 1 = 400 -> 255
     * Q should be set.
     */
    msr     CPSR_f, #0
    mov     r1, #200
    usat    r5, #8, r1, lsl #1
    mrs     r10, cpsr
    str     r5,  [r6, #32]
    str     r10, [r6, #36]

    bkpt    #0x1374
