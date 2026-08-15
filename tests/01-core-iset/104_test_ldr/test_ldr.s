.syntax unified
.arch armv7-a
.arm
.text
.global _start

/*
 * 104_test_ldr
 *
 * Behavioral validation for A32 LDR.
 *
 * We seed RAM with known words using STR, then verify LDR through:
 *
 *   1. [Rn]
 *   2. [Rn, #imm]
 *   3. post-index [Rn], #imm
 *   4. pre-index with writeback [Rn, #imm]!
 *   5. register offset [Rn, Rm]
 *
 * Results are copied to a separate result area so the harness can inspect
 * values and writeback behavior.
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

    /* Seed known words. */
    movw    r0, #0x3344
    movt    r0, #0x1122
    str     r0, [r6, #0]

    movw    r0, #0x7788
    movt    r0, #0x5566
    str     r0, [r6, #4]

    movw    r0, #0xBBCC
    movt    r0, #0x99AA
    str     r0, [r6, #8]

    movw    r0, #0xFF00
    movt    r0, #0xDDEE
    str     r0, [r6, #12]

    movw    r0, #0xBEEF
    movt    r0, #0xCAFE
    str     r0, [r6, #16]

    /* Case 1: plain [Rn]. */
    ldr     r1, [r6]
    str     r1, [r7, #0]

    /* Case 2: immediate offset, base unchanged. */
    ldr     r2, [r6, #4]
    str     r2, [r7, #4]

    /* Case 3: post-index. Capture loaded value and updated base. */
    mov     r8, r6
    ldr     r3, [r8], #8
    str     r3, [r7, #8]
    str     r8, [r7, #12]

    /* Case 4: pre-index with writeback. */
    mov     r9, r6
    ldr     r4, [r9, #12]!
    str     r4, [r7, #16]
    str     r9, [r7, #20]

    /* Case 5: register offset. */
    mov     r10, #16
    ldr     r5, [r6, r10]
    str     r5, [r7, #24]

    bkpt    #0x1374
