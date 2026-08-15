    .syntax unified
    .arch armv7-a
    .arm
    .text
    .global _start

/* Expect your linker script (linker.ld) to set ENTRY(_start) and place at 0x00008000 */

_start:
    /* ----------------------------------------------------------
     * Setup: r0 = 2, r1 = 10
     * ---------------------------------------------------------- */
    mov     r0, #2
    mov     r1, #10

    /* ----- Case 1: C = 1 (from CMP r0,r0) ---------------------
     * CMP r0,r0  => N=0, Z=1, C=1, V=0 (CARRY SET)
     * RSC r2,r0,r1 computes: r2 = r1 - r0 - (1-C) = 10 - 2 - 0 = 8
     * Flags NOT updated by plain RSC (no 'S').
     * ---------------------------------------------------------- */
    cmp     r0, r0            @ set C=1
    mrs     r4, cpsr          @ snapshot flags after CMP (C should be 1)
    rsc     r2, r0, r1        @ r2 = 0x8

    /* ----- Case 2: C = 0 (from CMP r0,r1) ---------------------
     * CMP r0,r1  => 2 - 10 -> borrow => C=0 (N=1, Z=0; V depends on implementation)
     * RSC r3,r0,r1 => r3 = r1 - r0 - (1-C) = 10 - 2 - 1 = 7
     * ---------------------------------------------------------- */
    cmp     r0, r1            @ set C=0
    mrs     r5, cpsr          @ snapshot flags after CMP (C should be 0)
    rsc     r3, r0, r1        @ r3 = 0x7

    /* ----- Case 3: RSCS (flag-setting) with C = 0 --------------
     * RSCS updates N/Z/C/V like a subtract with carry-in.
     * With current C=0:
     *   r1 - r0 - (1 - C) = 10 - 2 - 1 = 7
     * After RSCS:
     *   N=0, Z=0, C=1 (no borrow), V=0
     * ---------------------------------------------------------- */
    rscs    r8, r0, r1        @ r8 = 0x7, flags updated by RSCS
    mrs     r6, cpsr          @ snapshot flags after RSCS (expect C=1, Z=0)

  /* ----- Case 4: Shifted-operand variant ---------------------
     * C=1 carried in from Case 3.
     * r1, LSL #1 = 20
     * r11 = 20 - 2 - 0 = 18 (0x12)
     * RSCS sets flags; capture them.
     * ---------------------------------------------------------- */
    rscs    r11, r0, r1, lsl #1   @ r11 = 0x00000012
    mrs     r7, cpsr              @ snapshot flags after shifted RSCS

    /* Sentinel & stop */
    ldr     r12, =0xDEADBEEF  @ keep a known literal for your logger to show LDR literal works
    bkpt    #0x1234           @ stop/exit for your emulator
