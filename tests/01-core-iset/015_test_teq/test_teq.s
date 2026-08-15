    .text
    .global _start

/* TEQ / EOR flag-behavior test
 *
 * Covers:
 *   - TEQ with equal and non-equal registers
 *   - TEQ with MSB set (N flag)
 *   - TEQ with immediate operands (#0xFF, #0x80000000)
 *   - TEQ with shifted register (LSL/LSR/ASR/RRX, immediate shifts)
 *   - TEQ with register-shifted register (LSL r2)
 *   - CMP vs TEQ CPSR differences
 *   - Complex value via ADDS then TEQ #0
 *   - Conditional TEQEQ
 *   - Literal load + BKPT
 */

_start:
    /* --- 1. Basic TEQ cases: equal vs non-equal --- */

    mov     r0, #0xF0           @ r0 = 0xF0
    mov     r1, #0xF0           @ r1 = 0xF0
    teq     r0, r1              @ 0xF0 ^ 0xF0 = 0, Z=1
    mrs     r4, cpsr            @ r4: TEQ equal flags

    mov     r0, #0xAA           @ r0 = 0xAA
    mov     r1, #0x0F           @ r1 = 0x0F
    teq     r0, r1              @ 0xAA ^ 0x0F = 0xA5, Z=0
    mrs     r5, cpsr            @ r5: TEQ non-equal flags

    /* --- 2. MSB set + CMP baseline --- */

    mov     r0, #1
    lsl     r0, r0, #31         @ r0 = 0x80000000
    mov     r1, #0
    teq     r0, r1              @ result = 0x80000000, N=1
    mrs     r6, cpsr            @ r6: TEQ 0x80000000 ^ 0

    cmp     r0, r0              @ CMP baseline
    mrs     r7, cpsr            @ r7: CMP flags

    /* --- 3. TEQ with rotated immediates --- */

    teq     r0, #0xFF           @ TEQ with small immediate
    mrs     r8, cpsr            @ r8: TEQ r0,#0xFF

    teq     r0, #0x80000000     @ TEQ with large rotated immediate
    mrs     r9, cpsr            @ r9: TEQ r0,#0x80000000

    /* --- 4. Shifted register (immediate shifts) --- */

    mov     r1, #1
    lsl     r1, r1, #31         @ r1 = 0x80000000
    teq     r0, r1, lsl #1      @ LSL #1
    mrs     sl, cpsr            @ sl: TEQ LSL#1

    mov     r1, #1
    lsl     r1, r1, #31         @ r1 = 0x80000000
    teq     r0, r1, lsr #32     @ LSR #32
    mrs     fp, cpsr            @ fp: TEQ LSR#32

    mov     r1, #0
    teq     r0, r1, asr #32     @ ASR #32
    mrs     ip, cpsr            @ ip: TEQ ASR#32

    mov     r1, #1
    teq     r0, r1, rrx         @ RRX
    mrs     lr, cpsr            @ lr: TEQ RRX

    /* --- 5. Register-shifted register (different decode path) --- */

    mov     r0, #0x0F           @ r0 = 0x0F
    mov     r1, #1              @ base to shift
    mov     r2, #3              @ shift amount
    teq     r0, r1, lsl r2      @ operand2 = 1<<3 = 8, result = 0x0F ^ 0x08
    mrs     r10, cpsr           @ r10: TEQ reg-shift-reg

    /* --- 6. ADDS + TEQ #0 (complex path) --- */

    mov     r2, #0x7F
    lsl     r2, r2, #24         @ r2 = 0x7F000000
    adds    r2, r2, #0x81000000 @ exercise ADDS flags
    mrs     r3, cpsr            @ r3: CPSR after ADDS

    teq     r2, #0              @ TEQ with zero
    mrs     r2, cpsr            @ r2: TEQ r2,#0

    /* --- 7. Conditional TEQEQ --- */

    cmp     r0, r1              @ r0=0x0F, r1=1 -> Z=0
    teqeq   r0, r1              @ should NOT execute
    mrs     r1, cpsr            @ r1: confirm conditional behavior

    /* --- 8. Literal load + BKPT terminator --- */

    ldr     sp, =0xDEADBEEF
    bkpt    #0x1234
