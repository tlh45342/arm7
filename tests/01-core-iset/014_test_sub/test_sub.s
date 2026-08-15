    .text
    .global _start

/* 
 * Comprehensive SUB / SUBS test
 *
 * Goals:
 *   - Immediate SUB and SUBS
 *   - Register SUB and SUBS
 *   - Shifted operand2 (LSL/LSR/ASR/ROR) with immediate shift
 *   - Register-shifted register operand2
 *   - Flag behavior: N, Z, C, V for:
 *       * zero result
 *       * negative result
 *       * borrow / no-borrow
 *       * signed overflow
 *   - Conditional execution: EQ/NE, GE/LT
 *   - CPSR snapshots stored in r4–r9, r10 for inspection
 *
 * Convention:
 *   - Values we care about go into r0–r3, r4–r10.
 *   - CPSR snapshots are taken via MRS after interesting operations.
 *   - Final BKPT ends the test.
 */

_start:
    /* ------------------------------------------------------------
     * Section 1: Basic immediate SUB / SUBS and flag snapshots
     * ---------------------------------------------------------- */

    mov     r0, #5          @ r0 = 5
    mov     r1, #3          @ r1 = 3
    mov     lr, #0          @ clear LR so changes are visible

    cmp     r0, r0          @ 5 - 5 = 0 -> Z=1, N=0, C=1, V=0
    mrs     r4, cpsr        @ r4 = CPSR after cmp(r0,r0)

    sub     r2, r0, #2      @ r2 = 5 - 2 = 3; flags unchanged
    mrs     r5, cpsr        @ r5 should be same as r4

    subs    r3, r0, #7      @ r3 = 5 - 7 = -2 -> negative, borrow
    mrs     r6, cpsr        @ r6 = CPSR after negative SUBS

    mov     r2, #123        @ r2 = 123
    subs    ip, r2, #123    @ ip = 0 -> Z=1, N=0
    mrs     r7, cpsr        @ r7 = CPSR after zero result

    /* ------------------------------------------------------------
     * Section 2: Shifted immediate operand2 variants
     * (same spirit as your original test)
     * ---------------------------------------------------------- */

    mov     r6, #1
    lsl     r6, r6, #31     @ r6 = 0x80000000
    subs    sl, r6, #1      @ sl = 0x7FFFFFFF (overflow case)
    mrs     r8, cpsr        @ r8 = CPSR after 0x80000000 - 1

    /* plain register SUB */
    subs    fp, r0, r1      @ fp = 5 - 3 = 2
    mrs     r9, cpsr        @ r9 = CPSR after simple reg SUB

    mov     r1, #8
    subs    fp, r0, r1, lsl #1  @ fp = 5 - (8<<1) = 5 - 16 = -11
    mrs     r9, cpsr            @ update r9 (LSL case)

    mov     r1, #1
    lsl     r1, r1, #31         @ r1 = 0x80000000
    subs    fp, r6, r1, lsr #1  @ operand2 = 0x40000000, 0x80000000-0x40000000
    mrs     r9, cpsr            @ LSR case

    mov     r1, #0xFF
    subs    fp, r1, r6, asr #31 @ r6 ASR #31 = 0xFFFFFFFF, 0xFF - (-1)
    mrs     r9, cpsr            @ ASR case

    mov     r1, #18
    subs    fp, r0, r1, ror #4  @ ROR case on immediate
    mrs     r9, cpsr            @ ROR case snapshot

    /* ------------------------------------------------------------
     * Section 3: Classic edge cases for C/V (borrow/no-borrow)
     * ---------------------------------------------------------- */

    /* Case A: 0 - 1: should borrow, C=0, N=1, Z=0, V=0 */
    mov     r0, #0
    mov     r1, #1
    subs    r2, r0, r1          @ r2 = 0xFFFFFFFF
    mrs     r6, cpsr            @ r6 = CPSR: borrow case

    /* Case B: 0x80000000 - 1: overflow, no borrow, C=1, N=0, V=1 */
    ldr     r0, =0x80000000
    mov     r1, #1
    subs    r2, r0, r1          @ r2 = 0x7FFFFFFF
    mrs     r7, cpsr            @ r7 = CPSR: overflow case

    /* Case C: 0x80000000 - 0x80000000: zero, C=1, Z=1, V=0 */
    ldr     r0, =0x80000000
    ldr     r1, =0x80000000
    subs    r2, r0, r1          @ r2 = 0
    mrs     r8, cpsr            @ r8 = CPSR: equal-large-values

    /* ------------------------------------------------------------
     * Section 4: Register-shifted register operand2
     *   subs r3, r4, r5, lsl r6
     * ---------------------------------------------------------- */

    mov     r4, #0x10           @ 16
    mov     r5, #1              @ base to shift
    mov     r6, #2              @ shift amount = 2

    /* operand2 = r5 << r6 = 1 << 2 = 4, result = 16 - 4 = 12 */
    subs    r3, r4, r5, lsl r6  @ r3 = 12
    mrs     r9, cpsr            @ r9 = CPSR after reg-shift-reg

    /* Another reg-shift-reg with negative result */
    mov     r4, #2
    mov     r5, #4
    mov     r6, #1              @ operand2 = 4 << 1 = 8, 2 - 8 = -6
    subs    r3, r4, r5, lsl r6
    mrs     r10, cpsr           @ r10 = CPSR for negative reg-shift-reg

    /* ------------------------------------------------------------
     * Section 5: Conditional SUB tests (EQ/NE/GE/LT)
     * ---------------------------------------------------------- */

    /* Recreate your original EQ/NE behavior */

    mov     r0, #5
    mov     r1, #8
    cmp     r0, r1              @ r0 < r1: Z=0, N=1, sets LT
    subeq   lr, r0, r1          @ should NOT execute
    subne   lr, r0, r1          @ should execute: lr = 5 - 8 = -3

    /* Now test GE/LT explicitly, without changing flags (no 'S') */

    mov     r2, #0              @ r2 will record which path was taken

    /* r0 = 5, r1 = 3, so GE is true, LT is false */
    mov     r0, #5
    mov     r1, #3
    cmp     r0, r1              @ 5 - 3 -> GE
    mov     r3, #0
    subge   r3, r0, r1          @ r3 = 2 if GE taken
    sublt   r3, r1, r0          @ should NOT execute
    mov     r2, r3              @ r2 = 2 if GE worked

    /* r0 = 3, r1 = 5, so LT is true, GE is false */
    mov     r0, #3
    mov     r1, #5
    cmp     r0, r1              @ 3 - 5 -> LT
    mov     r3, #0
    subge   r3, r0, r1          @ should NOT execute now
    sublt   r3, r1, r0          @ r3 = 2 if LT taken
    mov     r2, r2              @ keep prior result in r2
    mov     r1, r3              @ r1 = 2 if LT path correct

    /* ------------------------------------------------------------
     * Section 6: Final literal load + BKPT to terminate
     * ---------------------------------------------------------- */

    ldr     sp, =0xDEADBEEF     @ simple literal load (tests LDR too)
    bkpt    #0x1234             @ end of test
