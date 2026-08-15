.syntax unified
.arch armv7-a
.arm
.text
.global _start

/*
 * 107_test_strb
 *
 * Behavioral validation for A32 STRB.
 *
 * Addressing forms:
 *   1. [Rn]
 *   2. [Rn, #imm]
 *   3. post-index [Rn], #imm
 *   4. pre-index with writeback [Rn, #imm]!
 *   5. register offset [Rn, Rm]
 *
 * Byte values are stored in DATA_BASE. LDRB is then used only to observe
 * the resulting bytes and copy them to RESULT_BASE for verification.
 */

.equ DATA_BASE,   0x00100000
.equ RESULT_BASE, 0x00100100

_start:
    /* r6 = DATA_BASE */
    movw    r6, #0x0000
    movt    r6, #0x0010

    /* r7 = RESULT_BASE */
    movw    r7, #0x0100
    movt    r7, #0x0010

    /* Case 1: plain [Rn]. */
    mov     r0, #0x11
    strb    r0, [r6]

    /* Case 2: immediate offset. */
    mov     r1, #0x22
    strb    r1, [r6, #1]

    /* Case 3: post-index. */
    mov     r2, #0x33
    add     r8, r6, #2
    strb    r2, [r8], #1
    str     r8, [r7, #20]      /* expected DATA_BASE + 3 */

    /* Case 4: pre-index with writeback. */
    mov     r3, #0x44
    mov     r9, r6
    strb    r3, [r9, #3]!
    str     r9, [r7, #24]      /* expected DATA_BASE + 3 */

    /* Case 5: register offset. */
    mov     r4, #0x55
    mov     r10, #4
    strb    r4, [r6, r10]

    /*
     * Read back stored bytes. LDRB is already independently verified.
     */
    ldrb    r11, [r6, #0]
    str     r11, [r7, #0]

    ldrb    r11, [r6, #1]
    str     r11, [r7, #4]

    ldrb    r11, [r6, #2]
    str     r11, [r7, #8]

    ldrb    r11, [r6, #3]
    str     r11, [r7, #12]

    ldrb    r11, [r6, #4]
    str     r11, [r7, #16]

    bkpt    #0x1374
