.syntax unified
.arch armv7-a
.arm
.text
.global _start

/*
 * 151_test_nop
 *
 * Proves NOP has no visible register/memory side effect and execution
 * continues to the next instruction.
 */
_start:
    movw    r6, #0x0000
    movt    r6, #0x0010

    mov     r0, #0x11
    str     r0, [r6, #0]

    nop

    mov     r1, #0x22
    str     r1, [r6, #4]

    bkpt    #0x1374
