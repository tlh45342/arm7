    .syntax unified
    .text
    .align  2
    .global _start

/* Comprehensive TST validation
   Assumptions:
     - Code loads at 0x00008000 (your harness already logs PC + disasm)
     - Logger prints MRS snapshots (so Python can assert NZCV precisely)
     - BKPT halts the run
*/

_start:
    /* --- 0) Init ---------------------------------------------------------- */
    mov     r14, #0            @ sentinel for conditional tests

    /* --- 1) Basic result cases (Z/N from rN & op2) ----------------------- */
    mov     r0, #0xF0
    mov     r1, #0x00          @ r0 & r1 = 0 -> Z=1, N=0
    tst     r0, r1
    mrs     r4, cpsr

    mov     r0, #0xAA
    mov     r1, #0x0F          @ 0xAA & 0x0F = 0x0A -> Z=0, N=0
    tst     r0, r1
    mrs     r5, cpsr

    mov     r0, #1
    lsl     r0, r0, #31        @ r0 = 0x80000000
    mov     r1, #0xF0
    lsl     r1, r1, #24        @ r1 = 0xF0000000 ; r0&r1 has msb=1 -> N=1, Z=0
    tst     r0, r1
    mrs     r6, cpsr

    /* --- 2) Immediate rotate == 0 preserves C ---------------------------- */
    mov     r0, #5
    mov     r1, #3
    cmp     r1, r0             @ 3-5 -> borrow -> C=0
    mrs     r7, cpsr           @ C=0 snapshot
    tst     r0, #0xFF          @ rotate 0 -> C must stay 0
    mrs     r8, cpsr

    cmp     r0, r0             @ 5-5 -> C=1
    mrs     r9, cpsr           @ C=1 snapshot
    tst     r0, #0x0F          @ rotate 0 -> C must stay 1
    mrs     r10, cpsr

    /* --- 3) Immediate with rotate > 0 sets C = bit31(rotated imm) -------- */
    @ choose two immediates that assemble with non-zero rotate:
    tst     r0, #0x80000000    @ encodes with rotate>0; C should become 1
    mrs     r11, cpsr
    tst     r0, #0x20000000    @ encodes with rotate>0; top bit 0 -> C should become 0
    mrs     r12, cpsr

    /* --- 4) Shifter carry from register shifts (fixed counts) ------------ */
    mov     r2, #1
    lsl     r2, r2, #31        @ r2 msb=1
    tst     r0, r2, lsl #1     @ carry-out = old bit31 -> C=1
    mrs     r3, cpsr

    mov     r2, #1             @ bit0=1
    tst     r0, r2, lsr #1     @ carry-out = old bit0 -> C=1
    mrs     r3, cpsr

    mov     r2, #0
    mvn     r2, r2             @ r2 = 0xFFFFFFFF (msb = 1)
    tst     r0, r2, asr #31    @ carry-out = old bit31 -> C=1
    mrs     r3, cpsr

    mov     r2, #1             @ RRX: carry-out = old bit0
    tst     r0, r2, rrx
    mrs     r3, cpsr

    /* --- 5) Variable shift amounts (register-specified counts) ----------- */
    mov     r2, #1
    mov     r3, #1
    lsl     r3, r3, #31        @ r3 msb=1
    tst     r0, r3, lsl r2     @ shift by 1 -> carry-out old bit31 -> C=1
    mrs     r1, cpsr

    mov     r2, #31
    mov     r3, #1
    lsl     r3, r3, #31        @ r3 msb=1
    tst     r0, r3, lsr r2     @ shift by 31 -> carry-out old bit30 (=0) -> C=0
    mrs     r1, cpsr

    /* --- 6) V must be unchanged by TST ----------------------------------- */
    mov     r2, #0x7F
    lsl     r2, r2, #24        @ r2 = 0x7F000000
    adds    r2, r2, #0x81000000 @ overflow -> V=1 (and probably C set)
    mrs     r0, cpsr           @ snapshot with V=1
    tst     r2, #0xF0          @ TST must not touch V
    mrs     r0, cpsr           @ V still 1

    /* --- 7) Conditional execution must skip and leave flags untouched ---- */
    mov     r0, #5
    mov     r1, #8
    cmp     r0, r1             @ Z=0
    tsteq   r0, r1             @ skipped (EQ false)
    mrs     r13, cpsr          @ unchanged
    cmp     r0, r0             @ Z=1
    tstne   r0, r1             @ skipped (NE false)
    mrs     r13, cpsr          @ unchanged again

    /* --- 8) Literal + halt for harness ----------------------------------- */
    ldr     r13, =0xDEADBEEF
    bkpt    #0x1234
