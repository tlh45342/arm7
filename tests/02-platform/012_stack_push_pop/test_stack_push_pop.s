.syntax unified
.arch armv7-a
.arm
.text
.global _start
_start:
    movw sp, #0xF000
    movt sp, #0x001D
    mov r8, sp

    movw r0, #0x1111
    movt r0, #0x1111
    movw r1, #0x2222
    movt r1, #0x2222
    movw r2, #0x3333
    movt r2, #0x3333
    movw r3, #0x4444
    movt r3, #0x4444
    movw lr, #0xEEEE
    movt lr, #0xEEEE

    push {r0-r3, lr}
    mov r9, sp

    mov r0, #0
    mov r1, #0
    mov r2, #0
    mov r3, #0
    mov lr, #0
    pop {r0-r3, lr}

    cmp sp, r8
    bne fail
    movw r10, #0x1111
    movt r10, #0x1111
    cmp r0, r10
    bne fail
    movw r10, #0x2222
    movt r10, #0x2222
    cmp r1, r10
    bne fail
    movw r10, #0x3333
    movt r10, #0x3333
    cmp r2, r10
    bne fail
    movw r10, #0x4444
    movt r10, #0x4444
    cmp r3, r10
    bne fail
    movw r10, #0xEEEE
    movt r10, #0xEEEE
    cmp lr, r10
    bne fail

    movw r11, #0x4B4F
    movt r11, #0x4B53      @ "SKOK"
    bkpt #0x120
fail:
    movw r11, #0x4146
    movt r11, #0x4C49      @ failure marker
    bkpt #0x121
