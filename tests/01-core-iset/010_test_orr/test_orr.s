    .syntax unified
    .text
    .global _start
    .type _start, %function

_start:
    // Original test: ORR (imm/reg), ORRS with shift, and conditional execution

    // r0 = 0x000000F0
    mov     r0, #0xF0

    // r1 = r0 OR #0xF = 0x000000FF
    orr     r1, r0, #0xF

    // r2 = 0x0000000F
    mov     r2, #0xF

    // r3 = r1 OR r2 = 0x000000FF
    orr     r3, r1, r2

    // r4 = 0x00000000 → r4 = r4 OR #1 = 0x00000001
    mov     r4, #0
    orr     r4, r4, #1

    // r5 = 1
    mov     r5, #1

    // ORRS with shift: r6 = r4 OR (r5 << 1) = 1 | 2 = 3
    // Sets flags from result (should be N=0, Z=0, C=0, V unchanged)
    orrs    r6, r4, r5, lsl #1

    // Snapshot CPSR after ORRS into r10
    mrs     r10, CPSR

    // These depend on Z==0 (from ORRS result 3), so they should NOT execute
    orreq   r7, r7, #0xFF
    moveq   r7, #1

    // --------------------------------------------------------------------
    // New Test 1: ORR with rotated immediate (0x80000000)
    // --------------------------------------------------------------------
    // r8 starts at 0, then ORR with 0x80000000 (requires rotated-imm encoding)
    mov     r8, #0
    orr     r8, r8, #0x80000000     // expect r8 = 0x80000000

    // --------------------------------------------------------------------
    // New Test 2: ORRS (immediate) setting N via high bit
    // --------------------------------------------------------------------
    // r9 starts at 0, ORRS with 0x80000000
    // Result: r9 = 0x80000000, N=1, Z=0. C comes from shifter (no shift → carry=old C).
    mov     r9, #0
    orrs    r9, r9, #0x80000000
    mrs     r11, CPSR              // capture CPSR after ORRS immediate

    // --------------------------------------------------------------------
    // New Test 3: ORR with ROR shift
    // --------------------------------------------------------------------
    // r12 = r0 OR (1 ROR #1)
    // 1 ROR 1 = 0x80000000 → result = 0x800000F0
    mov     r12, #1
    orr     r12, r0, r12, ror #1   // expect r12 = 0x800000F0

halt:
    bkpt    #0x1234
