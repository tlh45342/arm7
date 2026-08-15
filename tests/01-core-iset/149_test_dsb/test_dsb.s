.syntax unified
    .arm
    .global _start
_start:
    /* r6 = 0x0010_0000 (results base) */
    movw    r6, #0x0000
    movt    r6, #0x0010

    /* snapshot CPSR, then check it stays identical across DSBs via XOR==0 */
    mrs     r0, cpsr
    str     r0, [r6, #0]        @ optional: keep initial CPSR

    /* DSB #1: SY */
    dsb     sy
    mrs     r1, cpsr
    eor     r2, r0, r1
    str     r2, [r6, #4]        @ expect 0x00000000

    /* DSB #2: ST */
    dsb     st
    mrs     r3, cpsr
    eor     r4, r0, r3
    str     r4, [r6, #8]        @ expect 0x00000000

    /* DSB #3: ISH (inner-shareable) */
    dsb     ish
    mrs     r5, cpsr
    eor     r7, r0, r5
    str     r7, [r6, #12]       @ expect 0x00000000

    /* halt so the harness can read back */
    bkpt    #0x1234
