.syntax unified
.arch armv7-a
.arm
.text
.align 2
.global _start
.type _start, %function

/*
 * 171_test_mrrc2
 *
 * MRRC2 transfers a 64-bit coprocessor value into two ARM registers.
 *
 * This test is deliberately diagnostic:
 *   - it proves the instruction is assembled and reached;
 *   - it proves the decoder recognizes MRRC2;
 *   - it refuses to treat an unknown instruction silently advanced as NOP
 *     as a successful MRRC2 implementation.
 *
 * CP15 is used here because the inherited test used it. Once ARM7 has an
 * explicit coprocessor model, this test can be extended to validate concrete
 * returned values.
 */

.macro LOAD32 reg, imm
    movw    \reg, #:lower16:(\imm)
    movt    \reg, #:upper16:(\imm)
.endm

.equ BASE, 0x00100000

_start:
    LOAD32  r6, BASE

    /* Known pre-state. */
    LOAD32  r0, 0xAAAAAAAA
    LOAD32  r1, 0x55555555
    str     r0, [r6, #0x00]
    str     r1, [r6, #0x04]

    /*
     * Instruction under test.
     *
     * We do not currently assert the returned CP15 value here. The first
     * milestone is correct decode/dispatch rather than silent NOP behavior.
     */
    mrrc2   p15, #0, r0, r1, c14

    /* Reaching these stores proves execution returned from the handler. */
    str     r0, [r6, #0x08]
    str     r1, [r6, #0x0C]

    mrs     r7, cpsr
    str     r7, [r6, #0x10]

    bkpt    #0x1234

.size _start, . - _start
