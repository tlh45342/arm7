    .syntax unified
    .arch armv7-a
    .text
    .global _start

/* Conventions:
   - After any RSBS, we snapshot CPSR into r4..r8 (s1..s5).
   - We keep distinct result regs to avoid clobber surprises.
   - End with BKPT (0xE1212374) so your logger halts & dumps regs.
*/

_start:
    /* Base values */
    mov     r0, #5              @ r0 = 5
    mov     r1, #20             @ r1 = 20

    /* A) RSB no-flags: r2 = r1 - r0 = 15 */
    rsb     r2, r0, r1          @ r2 = 20 - 5 = 15

    /* B) RSBS negative: r3 = r0 - r1 = -15; N=1, Z=0, C=0, V=0 */
    rsbs    r3, r1, r0          @ r3 = 5 - 20 = 0xFFFFFFF1
    mrs     r4, cpsr            @ s1 = flags after B

    /* C) RSBS zero: r9 = 123 - 123 = 0; Z=1, C=1, N=0, V=0 */
    mov     r2, #123
    mov     r3, #123
    rsbs    r9, r2, r3          @ r9 = 123 - 123 = 0
    mrs     r5, cpsr            @ s2

	/* D) RSBS overflow example: (0x80000000 - 1) -> 0x7FFFFFFF; V=1, C=1 */
    mov     r8, #1              @ Rn = 1
    mov     r6, #1
    lsl     r6, r6, #31         @ Rm(op2) = 0x80000000 (use r6, not r9)
    rsbs    r10, r8, r6         @ r10 = 0x80000000 - 1 = 0x7FFFFFFF
    mrs     r6, cpsr            @ s3 = flags after overflow (reuse r6 now as snapshot)

    /* E) RSBS with shifted reg: r11 = (r1<<1) - r0 = (20? no; we reset below) */
    mov     r0, #5
    mov     r1, #8
    rsbs    r11, r0, r1, lsl #1 @ r11 = 16 - 5 = 11
    mrs     r7, cpsr            @ s4

    /* F) RSBS with immediate: r12 = 5 - 5 = 0; Z=1, C=1 */
    mov     r0, #5
    rsbs    r12, r0, #5         @ r12 = 0
    mrs     r8, cpsr            @ s5

    /* G) Conditional execution:
         - Make sure RSBEQ is skipped (Z=0)
         - Make sure RSBNE is skipped when Z=1
         - Then do a RSBEQ that executes (Z=1)
    */
    ldr     r13, =0xDEADBEEF    @ sentinels to detect skip
    ldr     r14, =0xDEADBEEF

    cmp     r0, r1              @ 5 vs 8 -> Z=0
    rsbeq   r13, r0, r1         @ should SKIP; r13 stays 0xDEADBEEF

    cmp     r0, r0              @ Z=1
    rsbne   r14, r0, r1         @ should SKIP; r14 stays 0xDEADBEEF
    rsbeq   r14, r1, r0         @ should EXECUTE now (8 - 5 = 3)

    // Halt (sentinal)
    bkpt    #0x1234
	