.syntax unified
.arch armv7-a
.arm

/*
 * ARM7 RTC MMIO integration test.
 *
 * Reads the complete guest-visible RTC calendar tuple through normal ARM LDR
 * instructions and stores the values into ordinary RAM for the host harness.
 *
 * Results:
 *   0x00100000 YEAR
 *   0x00100004 MONTH
 *   0x00100008 DAY
 *   0x0010000C HOUR
 *   0x00100010 MINUTE
 *   0x00100014 SECOND
 *   0x00100018 STATUS
 */

.equ RTC_BASE, 0xF0002000
.equ RESULTS,  0x00100000

.text
.global _start

_start:
    movw    r0, #:lower16:RTC_BASE
    movt    r0, #:upper16:RTC_BASE

    movw    r1, #:lower16:RESULTS
    movt    r1, #:upper16:RESULTS

    ldr     r2, [r0, #0x10]       /* YEAR */
    str     r2, [r1, #0x00]

    ldr     r2, [r0, #0x14]       /* MONTH */
    str     r2, [r1, #0x04]

    ldr     r2, [r0, #0x18]       /* DAY */
    str     r2, [r1, #0x08]

    ldr     r2, [r0, #0x1C]       /* HOUR */
    str     r2, [r1, #0x0C]

    ldr     r2, [r0, #0x20]       /* MINUTE */
    str     r2, [r1, #0x10]

    ldr     r2, [r0, #0x24]       /* SECOND */
    str     r2, [r1, #0x14]

    ldr     r2, [r0, #0x0C]       /* STATUS */
    str     r2, [r1, #0x18]

    bkpt    #0x5254
