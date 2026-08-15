.syntax unified
    .arch armv7-a
    .text
    .global _start

/* CMP flag tests (ARM state)
   Writes CPSR after each CMP to:
     [r6+0x00] : equal (reg vs reg)         -> expect NZCV = 0b0110 => 0x60000000
     [r6+0x04] : 1 - 2 (imm)                -> expect NZCV = 0b1000 => 0x80000000
     [r6+0x08] : 0x80000000 - 1 (imm)       -> expect NZCV = 0b0001 (V=1, N=0,C=0,Z=0) => 0x10000000
     [r6+0x0C] : 0x10 - (1<<4) (reg, LSL#4) -> expect NZCV = 0b0110 => 0x60000000
     [r6+0x10] : 0xFFFFFFFF - 0 (imm)       -> expect NZCV = 0b1010 => 0xA0000000
*/

_start:
    /* Case 1: equal -> Z=1, C=1, N=0, V=0 */
    mov     r1, #0x12
    mov     r2, #0x12
    cmp     r1, r2
    mrs     r0, cpsr
    str     r0, [r6, #0]

    /* Case 2: 1 - 2 -> negative, borrow => N=1, C=0, Z=0, V=0 */
    mov     r0, #1
    cmp     r0, #2
    mrs     r1, cpsr
    str     r1, [r6, #4]

    /* Case 3: 0x80000000 - 1 -> signed overflow => V=1, C=0, N=0, Z=0 */
    movw    r0, #0
    movt    r0, #0x8000
    cmp     r0, #1
    mrs     r2, cpsr
    str     r2, [r6, #8]

    /* Case 4: 0x10 - (1<<4) -> equal using shifted register operand */
    mov     r2, #0x10
    mov     r3, #1
    cmp     r2, r3, lsl #4
    mrs     r4, cpsr
    str     r4, [r6, #12]

    /* Case 5: 0xFFFFFFFF - 0 -> negative, no borrow => N=1, C=1, Z=0, V=0 */
    mvn     r4, #0
    cmp     r4, #0
    mrs     r5, cpsr
    str     r5, [r6, #16]

    /* stop */
    .word   0xE1212374    /* BKPT #0x1234 in ARM state */
	