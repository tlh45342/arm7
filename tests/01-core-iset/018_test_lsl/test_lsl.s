    .syntax unified
    .cpu arm7tdmi
    .text
    .global _start

/* ---------------------------------------------------------------------
 * LSL/LSLS comprehensive test
 *
 * Conventions for this test:
 *   - We snapshot CPSR into various registers after key operations.
 *   - Results of shifts are usually left in r2 (or r0 for some cases).
 *   - r4..r12, r1, r3, etc. hold CPSR snapshots for your Python tests.
 *
 * You can map:
 *   r4, r5, r6, ... etc. -> "cpsr_*" in run_tests.py
 * --------------------------------------------------------------------- */

_start:
    /* Make LR a known value so we can test it later if desired. */
    mov     lr, #0

/* ---------------------------------------------------------------------
 * Group 0: Baseline CPSR
 * --------------------------------------------------------------------- */

    mov     r0, #5
    cmp     r0, r0             @ Z=1, N=0, C=1 (borrow not) per standard CMP
    mrs     r4, cpsr           @ r4 = baseline CPSR with Z=1

/* ---------------------------------------------------------------------
 * Group 1: LSLS (immediate) basic behavior
 * --------------------------------------------------------------------- */

    /* 1.1: Simple shift left by 1, small value, no carry out */
    mov     r0, #1             @ 0x00000001
    lsls    r2, r0, #1         @ r2 = 0x00000002, C=0, N=0, Z=0
    mrs     r5, cpsr           @ r5: flags after LSLS #1 on 0x1

    /* 1.2: Shift 0x80000000 left by 1: result becomes 0, carry=1, Z=1 */
    ldr     r0, =0x80000000
    lsls    r2, r0, #1         @ r2 = 0x00000000, C=1, Z=1, N=0
    mrs     r6, cpsr           @ r6: LSLS #1 on 0x80000000

    /* 1.3: Shift 0x40000000 left by 1: result=0x80000000, C=0, N=1, Z=0 */
    ldr     r0, =0x40000000
    lsls    r2, r0, #1         @ r2 = 0x80000000, C=0, N=1, Z=0
    mrs     r7, cpsr           @ r7: LSLS #1 on 0x40000000

    /* 1.4: Shift 0x00000000 left by 1: result=0, C=0, Z=1, N=0 */
    mov     r0, #0
    lsls    r2, r0, #1         @ r2 = 0x00000000, C=0
    mrs     r8, cpsr           @ r8: LSLS #1 on 0x0

/* ---------------------------------------------------------------------
 * Group 2: LSLS (immediate) with shift by 31
 * --------------------------------------------------------------------- */

    /* 2.1: 0x00000001 << 31 => 0x80000000, C=0, N=1, Z=0 */
    mov     r0, #1
    lsls    r2, r0, #31        @ r2 = 0x80000000
    mrs     r9, cpsr           @ r9: LSLS #31 on 0x1

    /* 2.2: 0x00000003 << 31 => 0x80000000, C=1 (bit1 of original), N=1, Z=0 */
    mov     r0, #3
    lsls    r2, r0, #31        @ r2 = 0x80000000, C=1
    mrs     r10, cpsr          @ r10: LSLS #31 on 0x3

/* ---------------------------------------------------------------------
 * Group 3: LSL (immediate, non-S) must NOT change flags
 * --------------------------------------------------------------------- */

    /* Create known CPSR, then do LSL (no S) and confirm CPSR unchanged. */
    mov     r0, #1
    movs    r1, r0             @ sets flags based on 1 (N=0, Z=0)
    mrs     r11, cpsr          @ r11: CPSR before plain LSL

    lsl     r2, r0, #5         @ NO S bit; CPSR must remain same
    mrs     r12, cpsr          @ r12: CPSR after plain LSL

    /* r11 and r12 should be identical in your test */

/* ---------------------------------------------------------------------
 * Group 4: LSLS with register shift amounts (r3)
 * Tests special ARM rules for 0, 1-31, 32, >32
 * --------------------------------------------------------------------- */

    /* Use base value 0x00000001 for clear, known behavior. */
    mov     r0, #1

    /* 4.1: shift amount = 0: result = R0, C unchanged. */
    mov     r3, #0
    lsls    r2, r0, r3         @ r2 = 0x00000001, C unchanged
    mrs     r1, cpsr           @ r1: LSLS r3=0

    /* 4.2: shift amount = 1: result=2, C=0, N=0, Z=0 */
    mov     r3, #1
    lsls    r2, r0, r3         @ r2 = 0x00000002
    mrs     r2, cpsr           @ r2: LSLS r3=1

    /* 4.3: shift amount = 31: result=0x80000000, C=0, N=1, Z=0 */
    mov     r3, #31
    lsls    r2, r0, r3         @ r2 = 0x80000000
    mrs     r3, cpsr           @ r3: LSLS r3=31

    /* 4.4: shift amount = 32: result=0, C = bit0(original) = 1 */
    mov     r3, #32
    lsls    r2, r0, r3         @ r2 = 0x00000000
    mrs     r14, cpsr          @ r14: LSLS r3=32 (C=1, Z=1, N=0)

    /* 4.5: shift amount = 32 on a different base where bit0=0 */
    mov     r0, #2             @ 0x00000002, bit0=0
    mov     r3, #32
    lsls    r2, r0, r3         @ r2 = 0x00000000, C=0
    mrs     r5, cpsr           @ r5: LSLS r3=32, base bit0=0

    /* 4.6: shift amount > 32: result=0, C=0 */
    mov     r0, #1
    mov     r3, #33
    lsls    r2, r0, r3         @ r2 = 0x00000000, C=0
    mrs     r6, cpsr           @ r6: LSLS r3=33

    /* 4.7: large shift via r3=0x100 (256) => >32, result=0, C=0 */
    mov     r0, #1
    mov     r3, #0
    orr     r3, r3, #256       @ r3 = 0x100
    lsls    r2, r0, r3
    mrs     r7, cpsr           @ r7: LSLS r3=0x100

    /* 4.8: r3=0xFF (255): still >32, result=0, C=0 */
    mov     r0, #1
    mov     r3, #255           @ 0xFF
    lsls    r2, r0, r3
    mrs     r8, cpsr           @ r8: LSLS r3=0xFF

/* ---------------------------------------------------------------------
 * Group 5: Conditional LSL (eq/ne) – when condition is true vs false
 * --------------------------------------------------------------------- */

    /* 5.1: Condition true (EQ) – instruction executes, result and flags change */

    mov     r0, #1
    mov     r1, #1
    cmp     r0, r1             @ Z=1 (EQ)
    mrs     r9, cpsr           @ r9: CPSR before lsleq

    mov     r2, #0x10
    lsleq   r2, r0, #1         @ executes: r2 = 2, result sets no flags (no S)
    mrs     r10, cpsr          @ r10: CPSR after lsleq (unchanged vs r9)

    /* Note: LSL without S does not change flags, even when condition true.
     * So r9 and r10 should match. But r2 changed from 0x10 → 0x2
     */

    /* 5.2: Condition false (NE) – instruction does NOT execute, CPSR and dest unchanged */

    mov     r0, #1
    mov     r1, #1
    cmp     r0, r1             @ Z=1 → NE is false
    mrs     r11, cpsr          @ r11: CPSR before lslne

    mov     r2, #0x20
    lslne   r2, r0, #1         @ should NOT execute, r2 remains 0x20
    mrs     r12, cpsr          @ r12: CPSR after (unchanged vs r11)

/* ---------------------------------------------------------------------
 * Group 6: LSLS on various patterns to test N/Z behavior specifically
 * --------------------------------------------------------------------- */

    /* 6.1: Positive number, result still positive, Z=0, N=0 */
    mov     r0, #3
    lsls    r2, r0, #1         @ r2 = 6
    mrs     r1, cpsr           @ r1: LSLS #1 on 3

    /* 6.2: Value that becomes zero after shift (0x80000000 << 1 = 0) */
    ldr     r0, =0x80000000
    lsls    r2, r0, #1         @ r2=0
    mrs     r2, cpsr           @ r2: LSLS #1 on 0x80000000 (duplicate but ok)

    /* 6.3: Mix of bits – 0xF0000001 << 1 => 0xE0000002, N=1, C=1 (old bit31) */
    ldr     r0, =0xF0000001
    lsls    r2, r0, #1
    mrs     r3, cpsr           @ r3: LSLS #1 on 0xF0000001

/* ---------------------------------------------------------------------
 * Group 7: Final CMP/LR sanity check & terminate
 * --------------------------------------------------------------------- */

    cmp     lr, lr             @ should set Z=1, N=0, C=1
    mrs     r4, cpsr           @ r4: final CPSR snapshot (you can also check)

    /* Stop the test in a consistent way:
     *  - Set SP to 0xDEADBEEF (via a labeled word)
     *  - Execute an ARM BKPT encoded as a raw word, so assembler
     *    doesn't complain about arm7tdmi not supporting it.
     */

    ldr     sp, =deadbeef_val  @ SP = &deadbeef_val
    ldr     sp, [sp]           @ SP = 0xDEADBEEF

    .word   0xE1212374         @ encoded BKPT 0x1234 in ARM (as in old tests)
deadbeef_val:
    .word   0xDEADBEEF
