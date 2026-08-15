    .syntax unified
    .arch armv7-a
    .text
    .global _start

/* ---------------------------------------------------------------------
 * MRS comprehensive test (CPSR readback, extended)
 *
 * Groups:
 *   G0: Initial CPSR snapshot
 *   G1: CMP equal            → Z=1, N=0, C=1, V=0  (0x60000000)
 *   G2: SUBS 0 - 1           → N=1, Z=0, C=0, V=0  (0x80000000)
 *   G3: ADDS 0x40000000*2    → N=1, Z=0, C=0, V=1  (0x90000000)
 *   G4: ADDS 0xFFFFFFFF + 1  → N=0, Z=1, C=1, V=0  (0x60000000)
 *   G5: CMP lr,lr            → Z=1, N=0, C=1, V=0  (0x60000000)
 *   G6: TST / TEQ patterns   → TST (zero), TEQ (negative)
 *   G7: MSR to CPSR_f        → Write custom NZCV flags, read with MRS
 *
 * Snapshots:
 *   r0 = initial CPSR
 *   r1 = CPSR after CMP equal
 *   r2 = CPSR after SUBS (0-1)
 *   r3 = CPSR after overflow ADDS
 *   r4 = CPSR after carry-out ADDS
 *   r5 = CPSR after CMP lr,lr
 *   r6 = CPSR after TST
 *   r7 = CPSR after TEQ
 *   r8 = CPSR saved before MSR
 *   r9 = CPSR after MSR flags write
 *
 * SP (r13) = 0xDEADBEEF at BKPT.
 * --------------------------------------------------------------------- */

_start:
    /* Make LR a known value. */
    mov     lr, #0

/* ---------------------------------------------------------------------
 * G0: Initial CPSR snapshot
 * --------------------------------------------------------------------- */

    mrs     r0, cpsr           @ r0 = initial CPSR on entry

/* ---------------------------------------------------------------------
 * G1: CMP equal → Z=1, N=0, C=1, V=0  (0x60000000)
 * --------------------------------------------------------------------- */

    mov     r10, #5            @ arbitrary non-zero
    cmp     r10, r10           @ equal compare
    mrs     r1, cpsr           @ r1 snapshot

/* ---------------------------------------------------------------------
 * G2: SUBS 0 - 1 → N=1, Z=0, C=0, V=0  (0x80000000)
 * --------------------------------------------------------------------- */

    mov     r11, #0
    subs    r11, r11, #1       @ 0 - 1 = 0xFFFFFFFF
    mrs     r2, cpsr           @ r2 snapshot

/* ---------------------------------------------------------------------
 * G3: ADDS overflow (0x40000000 + 0x40000000)
 *   → N=1, Z=0, C=0, V=1  (0x90000000)
 * --------------------------------------------------------------------- */

    ldr     r12, =0x40000000
    adds    r12, r12, r12      @ 0x40000000 + 0x40000000 = 0x80000000
    mrs     r3, cpsr           @ r3 snapshot

/* ---------------------------------------------------------------------
 * G4: ADDS with carry-out but no overflow
 *   0xFFFFFFFF + 1 = 0x00000000
 *   → N=0, Z=1, C=1, V=0  (0x60000000)
 * --------------------------------------------------------------------- */

    mvn     r10, #0            @ r10 = 0xFFFFFFFF
    adds    r10, r10, #1       @ wrap to 0x00000000, carry out
    mrs     r4, cpsr           @ r4 snapshot

/* ---------------------------------------------------------------------
 * G5: Final CMP on LR (sanity) → Z=1, N=0, C=1, V=0
 * --------------------------------------------------------------------- */

    cmp     lr, lr             @ self-compare
    mrs     r5, cpsr           @ r5 snapshot

/* ---------------------------------------------------------------------
 * G6: TST / TEQ patterns
 *
 *  - TST: AND, sets N/Z (C/V unchanged)
 *      TST 0xF0, #0x0F = 0   → Z=1, N=0
 *  - TEQ: XOR, sets N/Z (C/V unchanged)
 *      TEQ 0x80000000, #1 = 0x80000001 → N=1, Z=0
 * --------------------------------------------------------------------- */

    /* G6.1: TST gives zero result */
    mov     r10, #0xF0         @ 0x000000F0
    tst     r10, #0x0F         @ result 0 → Z=1, N=0
    mrs     r6, cpsr           @ r6 snapshot

    /* G6.2: TEQ gives a negative, non-zero result */
    ldr     r11, =0x80000000   @ r11 = 0x80000000
    teq     r11, #1            @ 0x80000000 XOR 1 = 0x80000001 (N=1, Z=0)
    mrs     r7, cpsr           @ r7 snapshot

/* ---------------------------------------------------------------------
 * G7: MSR to CPSR_f (flags only) and MRS readback
 *
 *   We:
 *     - Save current CPSR in r8.
 *     - Write a custom NZCV pattern via MSR (flags-only field):
 *         NZCV = 1010b → N=1,Z=0,C=1,V=0 → 0xA0000000
 *     - Read CPSR into r9 via MRS to confirm.
 *     - Restore original CPSR from r8 at the end.
 *
 *   NOTE: This requires MSR CPSR_f to be implemented.
 * --------------------------------------------------------------------- */

    /* Save current CPSR (after TEQ) */
    mrs     r8, cpsr

    /* Write custom flags pattern via MSR (flags-only field) */
    ldr     r10, =0xA0000000      @ desired NZCV pattern
    msr     cpsr_f, r10           @ write flags from register
    mrs     r9, cpsr              @ r9 = CPSR with custom flags

    /* Restore original CPSR flags so we don't confuse later tests */
    msr     cpsr_f, r8

/* ---------------------------------------------------------------------
 * Termination: set SP to 0xDEADBEEF and execute BKPT encoding
 * --------------------------------------------------------------------- */

    ldr     sp, =0xDEADBEEF    @ r13 = 0xDEADBEEF
    bkpt    0x1234
	
