    .syntax unified
    .text
    .align  2
    .global _start

/* Comprehensive ASR validation
   Assumptions:
     - Code loads at 0x00008000
     - Emulator logs disasm + "MRS cpsr" snapshots (NZCV in CPSR)
     - BKPT halts
*/

_start:
    /* --- 0) Setup / sentinel -------------------------------------------- */
    mov     r14, #0

    /* --- 1) Immediate shifts -------------------------------------------- */
    /* ASR #1 : zero -> zero, C carries out bit0 */
    mov     r0, #0
    movs    r1, r0, asr #1
    mrs     r2, cpsr

    /* ASR #1 : 0x8000_0000 -> 0xC000_0000 (negative), C=0 */
    movw    r0, #0x0000
    movt    r0, #0x8000
    movs    r1, r0, asr #1
    mrs     r2, cpsr

    /* ASR #31 : sign into all bits, C = old bit30 */
    movw    r0, #0x0000
    movt    r0, #0x8000             @ negative
    movs    r1, r0, asr #31         @ -> 0xFFFFFFFF, C=1
    mrs     r2, cpsr

    movw    r0, #0x0000
    movt    r0, #0x1000             @ positive
    movs    r1, r0, asr #31         @ -> 0x00000000, C=0
    mrs     r2, cpsr

    /* ASR #32 (encoded as #0): result is all sign bits; C = old bit31 */
    movw    r0, #0x0000
    movt    r0, #0x8000             @ negative
    movs    r1, r0, asr #32         @ -> 0xFFFFFFFF, C=1
    mrs     r2, cpsr

    movw    r0, #0x0000
    movt    r0, #0x1000             @ positive
    movs    r1, r0, asr #32         @ -> 0x00000000, C=0
    mrs     r2, cpsr

    /* --- 2) Register-specified shifts ----------------------------------- */
    /* Rs = 0 -> special: no shift; C preserved */
    cmp     r14, r14                 @ set C=1
    mrs     r3, cpsr                 @ before
    movw    r0, #0x5678
    movt    r0, #0x1234
    mov     r2, #0                   @ Rs = 0
    movs    r1, r0, asr r2           @ no shift, C must remain 1
    mrs     r3, cpsr                 @ after

    /* Rs = 8 : ordinary arithmetic shift */
    mov     r2, #8
    movs    r1, r0, asr r2
    mrs     r3, cpsr

    /* Rs = 32 : explicit 32 via register */
    movw    r0, #0x0000
    movt    r0, #0x8000             @ negative, msb=1
    mov     r2, #32
    movs    r1, r0, asr r2          @ -> all ones, C=1
    mrs     r3, cpsr

    /* Rs = 40 (>=32), negative -> all ones, C=1; positive -> 0, C=0 */
    movw    r0, #0x0000
    movt    r0, #0x8000             @ negative
    mov     r2, #40
    movs    r1, r0, asr r2
    mrs     r3, cpsr

    movw    r0, #0x0000
    movt    r0, #0x1000             @ positive
    mov     r2, #40
    movs    r1, r0, asr r2
    mrs     r3, cpsr

    /* Rs masking: only low 8 bits used */
    /* Rs = 0x100 -> 0 (preserve C) */
    cmp     r14, r14                 @ C=1 again for visibility
    mrs     r3, cpsr
    movw    r0, #0x9ABC
    movt    r0, #0xDEF0
    mov     r2, #0
    orr     r2, r2, #0x100
    movs    r1, r0, asr r2           @ no shift, C must stay 1
    mrs     r3, cpsr

    /* Rs = 0xFF (>=32), positive -> 0, C=0 */
    movw    r0, #0x7FFF
    movt    r0, #0x0000             @ positive
    mov     r2, #0xFF
    movs    r1, r0, asr r2
    mrs     r3, cpsr

    /* --- 3) Non-S form should not change flags --------------------------- */
    cmp     r14, r14                 @ set known flags (Z=1, C=1, etc.)
    mrs     r4, cpsr                 @ before
    movw    r0, #0xAAAA
    movt    r0, #0x5555
    mov     r1, r0, asr #1           @ plain MOV (no 'S')
    mrs     r5, cpsr                 @ after (must equal r4)

    /* --- 4) Canonical zero / negative results ---------------------------- */
    /* Zero: 0x00000001 ASR #1 -> 0x00000000, C=1, Z=1, N=0 */
    mov     r0, #1
    movs    r1, r0, asr #1
    mrs     r6, cpsr

    /* Negative: 0x80000000 ASR #1 -> 0xC0000000, C=0, N=1 */
    movw    r0, #0x0000
    movt    r0, #0x8000
    movs    r1, r0, asr #1
    mrs     r6, cpsr

    /* --- 5) Rd == Rm overlap -------------------------------------------- */
    movw    r5, #0x0001
    movt    r5, #0x8000             @ r5 = 0x80000001
    movs    r5, r5, asr #1          @ in-place shift; expect 0xC0000000, C=1
    mrs     r7, cpsr

    /* --- 6) Tiny conditional sanity: skip should not alter flags --------- */
    cmp     r14, r14                 @ Z=1
    moveq   r8, r5, asr #1           @ skipped (EQ true? careful: EQ true → executes; use NE to skip)
    cmp     r14, r0                  @ Z=0
    moveq   r8, r5, asr #1           @ skipped (EQ false)
    mrs     r9, cpsr

    /* --- 7) Literal + halt ----------------------------------------------- */
    ldr     r13, =0xDEADBEEF
    bkpt    #0x1234
