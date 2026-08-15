    .global _start
_start:
    /* --- Setup ------------------------------------------------------------ */
    MOV   r0, #5
    MOV   r1, #3

    /* === 1) Plain SBC does NOT change flags ============================== */
    CMP   r0, r0              @ sets C=1, Z=1
    MRS   r4, cpsr            @ save flags (C should be 1)
    SBC   r2, r0, r1          @ r2 = 5 - 3 - (1-1) = 2 ; flags must be unchanged
    MRS   r5, cpsr            @ should equal r4

    /* === 2) SBCS updates flags (C=0 path) ================================ */
    CMP   r1, r0              @ 3 - 5 => borrow -> C=0
    SBCS  r3, r0, r1          @ r3 = 5 - 3 - (1-0) = 1 ; flags: N=0 Z=0 C=1 V=0
    MRS   r6, cpsr

    /* === 3) Zero result & carry = 1 (no borrow) ========================== */
    MOV   r2, #0x7B
    MOV   r3, #0x7B
    CMP   r2, r3              @ sets C=1, Z=1
    SBCS  r12, r2, r3         @ 0x7B - 0x7B - 0 = 0 ; Z=1, C=1
    MRS   r7, cpsr

    /* === 4) Overflow case (V=1) ========================================== */
    MOV   r6, #1
    LSL   r6, r6, #31         @ r6 = 0x80000000
    MOV   r8, #1
    CMP   r8, r8              @ C=1
    SBCS  r10, r6, #1         @ 0x80000000 - 1 - 0 = 0x7FFFFFFF ; V=1, C=1
    MRS   r8, cpsr

    /* === 5) Shifted operand form ========================================= */
    CMP   r0, r0              @ ensure C=1 to isolate arithmetic C from shifter_carry
    MOV   r1, #8
    SBCS  r11, r0, r1, LSL #1 @ 5 - 16 - 0 = 0xFFFFFFF5 ; N=1, C=0
    MRS   r9, cpsr

    /* === 6) Conditional execution (should skip) ========================== */
    CMP   r0, r1              @ Z=0
    SBCEQ r14, r0, r1         @ skipped (EQ false), r14 unchanged
    CMP   r0, r0              @ Z=1
    SBCNE r14, r0, r1         @ skipped (NE false), r14 unchanged

    /* Sentinel & halt */
    LDR   r13, =0xDEADBEEF
    BKPT  #0x1234
