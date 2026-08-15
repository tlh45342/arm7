.syntax unified
.arch armv7-a
.arm
.text
.global _start

/*
 * 105_test_str
 *
 * Behavioral validation for A32 STR.
 *
 * Addressing forms:
 *   1. [Rn]
 *   2. [Rn, #imm]
 *   3. post-index [Rn], #imm
 *   4. pre-index with writeback [Rn, #imm]!
 *   5. register offset [Rn, Rm]
 *
 * Stores are made into DATA_BASE. LDR then copies the resulting words and
 * writeback values into RESULT_BASE so the harness verifies actual behavior.
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
    movw    r0, #0x3344
    movt    r0, #0x1122
    str     r0, [r6]

    /* Case 2: immediate offset, base unchanged. */
    movw    r1, #0x7788
    movt    r1, #0x5566
    str     r1, [r6, #4]

    /* Case 3: post-index. */
    movw    r2, #0xBBCC
    movt    r2, #0x99AA
    mov     r8, r6
    str     r2, [r8], #8

    /* Capture updated post-index base. */
    str     r8, [r7, #20]

    /* Case 4: pre-index with writeback. */
    movw    r3, #0xFF00
    movt    r3, #0xDDEE
    mov     r9, r6
    str     r3, [r9, #12]!

    /* Capture updated pre-index base. */
    str     r9, [r7, #24]

    /* Case 5: register offset. */
    movw    r4, #0xBEEF
    movt    r4, #0xCAFE
    mov     r10, #16
    str     r4, [r6, r10]

    /*
     * Read back all data locations.
     * This uses the already-verified LDR path only as an observation tool.
     */
    ldr     r11, [r6, #0]
    str     r11, [r7, #0]

    ldr     r11, [r6, #4]
    str     r11, [r7, #4]

    /*
     * Post-index case stored at DATA_BASE before r8 advanced, so this should
     * contain the case-3 value, replacing the original case-1 value.
     */
    ldr     r11, [r6, #0]
    str     r11, [r7, #8]

    ldr     r11, [r6, #12]
    str     r11, [r7, #12]

    ldr     r11, [r6, #16]
    str     r11, [r7, #16]

    bkpt    #0x1374
