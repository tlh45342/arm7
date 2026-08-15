.syntax unified
.arch armv7-a
.arm
.text
.align 2
.global _start

.equ BOOT_MARKER, 0x00030C00

_start:
    /*
     * This code exists only inside BOOT.BIN on disk.
     * If these markers appear, execution reached disk-loaded code.
     */
    movw r4, #:lower16:BOOT_MARKER
    movt r4, #:upper16:BOOT_MARKER

    movw r0, #0x4B4F
    movt r0, #0x5442       /* "OKBT" */
    str r0, [r4, #0]

    movw r0, #0xB007
    movt r0, #0x0001       /* distinctive 0x0001B007 */
    str r0, [r4, #4]

    adr r0, _start
    str r0, [r4, #8]       /* proves linked/running at 0x00010000 */

    movw r0, #0xCAFE
    movt r0, #0xB007
    str r0, [r4, #12]

    bkpt 0x7007

hang:
    b hang

/* Pad beyond one sector so the handoff also depends on multi-sector loading. */
.space 700 - (. - _start), 0xA5
