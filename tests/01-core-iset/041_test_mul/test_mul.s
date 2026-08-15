.text
    .syntax unified
    .cpu cortex-a9
    .arm
    .global _start

/* tests/041_test_mul.s (ARM state)
 * Cases:
 *  1) 0 * 123        -> 0        (Z=1)
 *  2) 3 * 7          -> 21       (Z=0, N=0)
 *  3) (-1) * 5       -> -5       (N=1)
 *  4) 0x12345678 * 2 -> 0x2468ACF0 (N=0)
 *  5) 0x80000000 * 2 -> 0x00000000 (wrap to 0, Z=1)
 * Notes: In ARM state, MULS updates N/Z. We zero C/V beforehand so flag snapshots are clean.
 */

_start:
    // Optional: base for storing results if you want (not required by harness)
    movw    r6, #0x0000
    movt    r6, #0x0010

    // Clear flags: r14=0; ADDS r14,r14,r14 -> Z=1, N=0, C=0, V=0
    mov     r14, #0
    adds    r14, r14, r14

    // -------- Case 1: 0 * 123 -> 0 (Z=1) --------
    mov     r2, #0
    mov     r3, #123
    muls    r0, r2, r3          // r0 = 0
    mrs     r8, cpsr            // expect Z=1 (bit30)

    // -------- Case 2: 3 * 7 -> 21 --------
    mov     r2, #3
    mov     r3, #7
    muls    r1, r2, r3          // r1 = 21 (0x15)
    mrs     r9, cpsr            // expect 0

    // -------- Case 3: (-1) * 5 -> -5 --------
    mvn     r2, #0              // r2 = 0xFFFFFFFF (-1)
    mov     r3, #5
    muls    r4, r2, r3          // r4 = 0xFFFFFFFB
    mrs     r10, cpsr           // expect N=1 (bit31)

    // -------- Case 4: 0x12345678 * 2 -> 0x2468ACF0 --------
    movw    r2, #0x5678
    movt    r2, #0x1234
    mov     r3, #2
    muls    r3, r2, r3          // r3 = 0x2468ACF0
    mrs     r11, cpsr           // expect 0

    // -------- Case 5: 0x80000000 * 2 -> 0x00000000 --------
    movw    r2, #0x0000
    movt    r2, #0x8000
    mov     r5, #2
    muls    r5, r2, r5          // r5 = 0
    mrs     r12, cpsr           // expect Z=1 (bit30)

    // Sentinel for harness
    bkpt    #0x1234
	