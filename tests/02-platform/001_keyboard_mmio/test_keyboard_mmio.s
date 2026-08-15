.syntax unified
.arch armv7-a
.arm

/*
 * Host injects 'A' before execution.
 *
 * Guest proves:
 *   KEY_STATUS READY becomes visible through MMIO
 *   KEY_DATA returns 0x41
 *   reading KEY_DATA drains the FIFO
 *
 * Results:
 *   0x00100000 = character read
 *   0x00100004 = STATUS after DATA read
 */

.equ KEY_STATUS, 0x0A001000
.equ KEY_DATA,   0x0A001004
.equ RESULTS,    0x00100000

.text
.global _start

_start:
    movw    r0, #:lower16:KEY_STATUS
    movt    r0, #:upper16:KEY_STATUS

wait_key:
    ldr     r1, [r0]
    tst     r1, #1
    beq     wait_key

    ldr     r2, [r0, #4]

    movw    r3, #:lower16:RESULTS
    movt    r3, #:upper16:RESULTS
    str     r2, [r3]

    /* DATA read should have popped the only byte. */
    ldr     r4, [r0]
    str     r4, [r3, #4]

    bkpt    #0x4B44
