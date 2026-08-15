.syntax unified
.arch armv7-a
.arm
.text
.global _start

/*
 * 108_test_ldrh
 *
 * Behavioral validation for A32 LDRH.
 *
 * Addressing forms:
 *   1. [Rn]
 *   2. [Rn, #imm]
 *   3. post-index [Rn], #imm
 *   4. pre-index with writeback [Rn, #imm]!
 *   5. register offset [Rn, Rm]
 *
 * Halfwords are seeded with STRH. Loaded values are copied to RESULT_BASE.
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

    /* Seed halfwords. */
    movw    r0, #0x1122
    strh    r0, [r6, #0]

    movw    r0, #0x3344
    strh    r0, [r6, #2]

    movw    r0, #0x5566
    strh    r0, [r6, #4]

    movw    r0, #0x7788
    strh    r0, [r6, #6]

    movw    r0, #0x99AA
    strh    r0, [r6, #8]

    /* Case 1: plain [Rn]. */
    ldrh    r1, [r6]
    str     r1, [r7, #0]

    /* Case 2: immediate offset. */
    ldrh    r2, [r6, #2]
    str     r2, [r7, #4]

    /* Case 3: post-index. */
    mov     r8, r6
    ldrh    r3, [r8], #4
    str     r3, [r7, #8]
    str     r8, [r7, #12]

    /* Case 4: pre-index with writeback. */
    mov     r9, r6
    ldrh    r4, [r9, #6]!
    str     r4, [r7, #16]
    str     r9, [r7, #20]

    /* Case 5: register offset. */
    mov     r10, #8
    ldrh    r5, [r6, r10]
    str     r5, [r7, #24]

    bkpt    #0x1374
