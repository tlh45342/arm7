    .syntax unified
    .cpu arm7tdmi
    .text
    .global _start

/* ---------------------------------------------------------------------
 * MSR comprehensive test (flags-only, CPSR_f)
 *
 * Groups:
 *   G0: Snapshot initial CPSR
 *   G1: MSR cpsr_f, #0x60000000  → Z=1, C=1 (NZCV = 0b0110)
 *   G2: MSR cpsr_f, #0x80000000  → N=1, Z=0, C=0, V=0
 *   G3: MSR cpsr_f, Rm (0xA0000000) → N=1, Z=0, C=1, V=0
 *   G4: MSR cpsr_f, #0x00000000  → all flags clear
 *   G5: MSR cpsr_f, Rm (0xF0000000 via MVN) → N=1, Z=1, C=1, V=1
 *   G6: Restore original CPSR flags from R0
 *
 * Snapshots:
 *   r0 = initial CPSR on entry
 *   r1 = CPSR after MSR #0x60000000
 *   r2 = CPSR after MSR #0x80000000
 *   r3 = CPSR after MSR Rm = 0xA0000000
 *   r4 = CPSR after MSR #0x00000000
 *   r5 = CPSR after MSR Rm = 0xF0000000 (flags all 1)
 *   r6 = CPSR after restoring original flags from r0
 *
 * SP (r13) = 0xDEADBEEF at BKPT.
 *
 * NOTE: This assumes your VM implements:
 *   - MRS rD, CPSR
 *   - MSR CPSR_f, #imm   (flags-only immediate)
 *   - MSR CPSR_f, Rm     (flags-only from register)
 * --------------------------------------------------------------------- */

_start:
    /* Make LR a known value (not strictly needed here, but consistent). */
    mov     lr, #0

/* ---------------------------------------------------------------------
 * G0: Snapshot initial CPSR
 * --------------------------------------------------------------------- */

    mrs     r0, cpsr           @ r0 = initial CPSR on entry

/* ---------------------------------------------------------------------
 * G1: MSR cpsr_f, #0x60000000
 *
 *   NZCV = 0b0110 → Z=1, C=1, N=0, V=0
 *   0x60000000 is encodable as an ARM immediate.
 * --------------------------------------------------------------------- */

    msr     cpsr_f, #0x60000000
    mrs     r1, cpsr           @ r1 snapshot

/* ---------------------------------------------------------------------
 * G2: MSR cpsr_f, #0x80000000
 *
 *   NZCV = 0b1000 → N=1, Z=0, C=0, V=0
 * --------------------------------------------------------------------- */

    msr     cpsr_f, #0x80000000
    mrs     r2, cpsr           @ r2 snapshot

/* ---------------------------------------------------------------------
 * G3: MSR cpsr_f, Rm (0xA0000000)
 *
 *   NZCV = 0b1010 → N=1, Z=0, C=1, V=0
 *   We load via literal and then use the register form of MSR.
 * --------------------------------------------------------------------- */

    ldr     r10, =0xA0000000
    msr     cpsr_f, r10
    mrs     r3, cpsr           @ r3 snapshot

/* ---------------------------------------------------------------------
 * G4: MSR cpsr_f, #0x00000000
 *
 *   Clear all condition flags.
 * --------------------------------------------------------------------- */

    msr     cpsr_f, #0x00000000
    mrs     r4, cpsr           @ r4 snapshot

/* ---------------------------------------------------------------------
 * G5: MSR cpsr_f, Rm (0xF0000000 via MVN)
 *
 *   We create a value with high bits 1 (0xFFFFFFFF) and rely on the
 *   implementation to mask to CPSR_f.
 *   Intended NZCV flags = 0b1111 → N=1, Z=1, C=1, V=1 (0xF0000000).
 * --------------------------------------------------------------------- */

    mvn     r11, #0            @ r11 = 0xFFFFFFFF
    msr     cpsr_f, r11
    mrs     r5, cpsr           @ r5 snapshot

/* ---------------------------------------------------------------------
 * G6: Restore original CPSR flags from r0
 *
 *   This ensures that any subsequent tests run in the same VM session
 *   won't inherit weird flags from this one.
 * --------------------------------------------------------------------- */

    msr     cpsr_f, r0
    mrs     r6, cpsr           @ r6 should match original flags in r0
                               @ (mode bits should never have changed)

/* ---------------------------------------------------------------------
 * Termination: set SP to 0xDEADBEEF and execute BKPT encoding
 * --------------------------------------------------------------------- */

    ldr     sp, =0xDEADBEEF    @ r13 = 0xDEADBEEF
    .word   0xE1212374         @ BKPT 0x1234 encoding in ARM (raw word)
