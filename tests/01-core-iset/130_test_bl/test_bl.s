.syntax unified
.arch armv7-a
.arm
.text
.global _start

/*
 * 130_test_bl
 *
 * Validates ARM BL:
 *   1. BL reaches the subroutine.
 *   2. LR contains the address of the instruction following BL.
 *   3. BX LR returns to the caller.
 *   4. Caller continues after return.
 *
 * Results at 0x00100000:
 *   +0x00 = 0x11111111  subroutine reached
 *   +0x04 = LR captured inside subroutine
 *   +0x08 = 0x22222222  caller resumed after return
 */

_start:
    movw    r6, #0x0000
    movt    r6, #0x0010

    mov     r0, #0
    mov     r1, #0

    bl      subroutine

after_bl:
    movw    r1, #0x2222
    movt    r1, #0x2222
    str     r1, [r6, #8]

    bkpt    #0x1374

subroutine:
    movw    r0, #0x1111
    movt    r0, #0x1111
    str     r0, [r6, #0]

    /* Capture LR so the harness can verify BL link semantics. */
    str     lr, [r6, #4]

    bx      lr
