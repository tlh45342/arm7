.syntax unified
.arch armv7-a
.arm
.text
.global _start

/*
 * 173_test_movw
 *
 * Dedicated behavioral validation for MOVW.
 *
 * Cases:
 *   r0 = 0x00001234
 *   r1 = 0x0000ABCD
 *   r2 = 0x00000000
 *   r3 = 0x0000FFFF
 *
 * Results are stored at 0x00100000.
 */

_start:
    movw    r6, #0x0000
    movt    r6, #0x0010

    movw    r0, #0x1234
    str     r0, [r6, #0]

    movw    r1, #0xABCD
    str     r1, [r6, #4]

    movw    r2, #0x0000
    str     r2, [r6, #8]

    movw    r3, #0xFFFF
    str     r3, [r6, #12]

    bkpt    #0x1374
