    .syntax unified
    .arch armv7-a
    .arm
    .text
    .align 2
    .global _start
    .type   _start, %function

/* ------------------------
   Helpers/macros
   ------------------------ */
    .macro LOAD32 reg, imm
        movw    \reg, #:lower16:(\imm)
        movt    \reg, #:upper16:(\imm)
    .endm

/* ------------------------
   Constants
   ------------------------ */
    .equ    BASE, 0x00100000    /* where we’ll dump results */

/* Layout per test case at BASE + N*0x10:
   [0x0] original (r0)
   [0x4] RBIT(r0) (r1)
   [0x8] CPSR after RBIT (r7)
   [0xC] reserved (0)  */

_start:
    /* r6 = BASE */
    LOAD32  r6, BASE

    /* -------- Case A: 0x00000001 → expect 0x80000000 -------- */
    LOAD32  r0, 0x00000001
    rbit    r1, r0
    mrs     r7, cpsr
    str     r0, [r6, #0x00]
    str     r1, [r6, #0x04]
    str     r7, [r6, #0x08]
    mov     r2, #0
    str     r2, [r6, #0x0C]

    /* -------- Case B: 0x80000000 → expect 0x00000001 -------- */
    LOAD32  r0, 0x80000000
    rbit    r1, r0
    mrs     r7, cpsr
    str     r0, [r6, #0x10]
    str     r1, [r6, #0x14]
    str     r7, [r6, #0x18]
    str     r2, [r6, #0x1C]

    /* -------- Case C: 0xF0F0F0F0 → expect 0x0F0F0F0F -------- */
    LOAD32  r0, 0xF0F0F0F0
    rbit    r1, r0
    mrs     r7, cpsr
    str     r0, [r6, #0x20]
    str     r1, [r6, #0x24]
    str     r7, [r6, #0x28]
    str     r2, [r6, #0x2C]

    /* -------- Case D: 0x01234567 (handy mixed pattern) ------- */
    LOAD32  r0, 0x01234567
    rbit    r1, r0
    mrs     r7, cpsr
    str     r0, [r6, #0x30]
    str     r1, [r6, #0x34]
    str     r7, [r6, #0x38]
    str     r2, [r6, #0x3C]

    /* Halt so the test harness can read memory */
    bkpt    0x1234

    .size _start, . - _start
