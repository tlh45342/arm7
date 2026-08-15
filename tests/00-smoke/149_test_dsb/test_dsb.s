.syntax unified
.arch armv7-a
.arm
.text
.global _start

/*
 * 149_test_dsb
 *
 * DSB is a synchronization barrier. In this VM we do not model caches,
 * store buffers, or an out-of-order memory hierarchy, so the meaningful
 * core test is:
 *
 *   - DSB is recognized/executed without fault
 *   - execution continues after DSB
 *
 * Memory markers prove instructions before and after the DSB executed.
 */
_start:
    movw    r6, #0x0000
    movt    r6, #0x0010

    mov     r0, #0x11
    str     r0, [r6, #0]

    dsb     sy

    mov     r1, #0x22
    str     r1, [r6, #4]

    bkpt    #0x1374
