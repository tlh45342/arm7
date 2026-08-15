.syntax unified
.arch armv7-a
.arm
.text
.align 2
.global _start

.equ SVC_PUTC, 1
.equ SVC_PUTS, 2
.equ SVC_EXIT, 3

_start:
    movw sp, #0xE000
    movt sp, #0x001E

    movw r0, #:lower16:msg1
    movt r0, #:upper16:msg1
    mov r7, #SVC_PUTS
    svc #0

    mov r7, #SVC_PUTC
    mov r0, #'A'
    svc #0
    mov r0, #'B'
    svc #0
    mov r0, #'C'
    svc #0
    mov r0, #10
    svc #0

    movw r0, #:lower16:msg2
    movt r0, #:upper16:msg2
    mov r7, #SVC_PUTS
    svc #0

    mov r0, #42
    mov r7, #SVC_EXIT
    svc #0

1:  b 1b

msg1: .asciz "SVC puts works\n"
msg2: .asciz "kernel owns console\n"
