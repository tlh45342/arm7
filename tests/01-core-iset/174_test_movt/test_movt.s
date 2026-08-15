.syntax unified
.arch armv7-a
.arm
.text
.global _start

/*
 * 174_test_movt
 *
 * Dedicated behavioral validation for MOVT.
 *
 * MOVT replaces bits [31:16] while preserving bits [15:0].
 *
 * Cases:
 *   0x00001234 + MOVT #0xABCD -> 0xABCD1234
 *   0x0000FFFF + MOVT #0x0000 -> 0x0000FFFF
 *   0x00000000 + MOVT #0xFFFF -> 0xFFFF0000
 *   0x00005678 + MOVT #0x1234 -> 0x12345678
 *
 * Results are stored at 0x00100000.
 */

_start:
    /* Result buffer. */
    movw    r6, #0x0000
    movt    r6, #0x0010

    /* Case 1: preserve 0x1234, install 0xABCD above it. */
    movw    r0, #0x1234
    movt    r0, #0xABCD
    str     r0, [r6, #0]

    /* Case 2: MOVT #0 clears only upper halfword. */
    movw    r1, #0xFFFF
    movt    r1, #0x0000
    str     r1, [r6, #4]

    /* Case 3: install all ones in upper halfword. */
    movw    r2, #0x0000
    movt    r2, #0xFFFF
    str     r2, [r6, #8]

    /* Case 4: classic 32-bit constant construction. */
    movw    r3, #0x5678
    movt    r3, #0x1234
    str     r3, [r6, #12]

    bkpt    #0x1374
