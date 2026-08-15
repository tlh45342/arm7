.syntax unified
.arch armv7-a
.arm
.text
.align 2
.global _start

.equ MONITOR_MARKER, 0x00030C40
.equ CRT_BASE,       0x0A000000
.equ CRT_ATTR,       0x07

_start:
    /*
     * This code exists only inside MONITOR.BIN on disk.
     * Unique RAM markers prove execution reached the loaded monitor.
     */
    movw r4, #:lower16:MONITOR_MARKER
    movt r4, #:upper16:MONITOR_MARKER

    movw r0, #0x4B4F
    movt r0, #0x4E4D          /* "OKMN" */
    str r0, [r4, #0]

    movw r0, #0x2007
    movt r0, #0x0002
    str r0, [r4, #4]

    adr r0, _start
    str r0, [r4, #8]          /* should be 0x00020000 */

    movw r0, #0xCAFE
    movt r0, #0x2007
    str r0, [r4, #12]

    /* Also prove visible monitor ownership of the CRT. */
    movw r1, #:lower16:CRT_BASE
    movt r1, #:upper16:CRT_BASE
    movw r0, #:lower16:banner
    movt r0, #:upper16:banner
    mov r3, #CRT_ATTR
1:
    ldrb r2, [r0], #1
    cmp r2, #0
    beq 2f
    strb r2, [r1]
    strb r3, [r1, #1]
    add r1, r1, #2
    b 1b
2:
    bkpt 0x7010

hang:
    b hang

banner:
    .asciz "DISK-LOADED MONITOR OK"

/* Force a 1148-byte file: three sectors required. */
.space 1148 - (. - _start), 0xA5
