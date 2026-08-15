.syntax unified
.arch armv7-a
.arm

.equ CRT_BASE,     0x0A000000
.equ CRT_ATTR,     0x07
.equ CRT_COLS,     80
.equ CRT_ROWS,     25
.equ CRT_CELLS,    (CRT_COLS * CRT_ROWS)

.equ CURSOR_STATE, 0x0003F000
.equ RESULT_STATE, 0x0003F004
.equ SAVE_R4,      0x0003F008
.equ SAVE_R5,      0x0003F00C

.equ SVC_PUTC, 1
.equ SVC_PUTS, 2
.equ SVC_EXIT, 3

.section .vectors, "ax"
.align 5
.global _start
_start:
    b reset
    b vector_undef
    b svc_entry
    b vector_pabort
    b vector_dabort
    b vector_reserved
    b vector_irq
    b vector_fiq

.section .text, "ax"
.align 2

reset:
    movw sp, #0xF000
    movt sp, #0x001F

    movw r4, #:lower16:CURSOR_STATE
    movt r4, #:upper16:CURSOR_STATE
    mov r0, #0
    str r0, [r4]
    str r0, [r4, #4]

    movw r4, #:lower16:CRT_BASE
    movt r4, #:upper16:CRT_BASE
    mov r0, #0x20
    mov r3, #CRT_ATTR
    movw r1, #CRT_CELLS
    mov r2, r4
clear_loop:
    strb r0, [r2]
    strb r3, [r2, #1]
    add r2, r2, #2
    subs r1, r1, #1
    bne clear_loop

    movw r0, #0x0000
    movt r0, #0x0001
    bx r0

svc_entry:
    cmp r7, #SVC_PUTC
    beq svc_putc
    cmp r7, #SVC_PUTS
    beq svc_puts
    cmp r7, #SVC_EXIT
    beq svc_exit
    mov r0, #-1
    movs pc, lr

svc_putc:
    mov r5, lr
    adr r12, svc_putc_done
    b kernel_putc

svc_putc_done:
    mov lr, r5
    movs pc, lr

svc_puts:
    movw r3, #:lower16:SAVE_R4
    movt r3, #:upper16:SAVE_R4
    str r4, [r3]
    str r5, [r3, #4]

    mov r5, lr
    mov r4, r0

svc_puts_loop:
    ldrb r0, [r4], #1
    cmp r0, #0
    beq svc_puts_done
    adr r12, svc_puts_after_char
    b kernel_putc

svc_puts_after_char:
    b svc_puts_loop

svc_puts_done:
    movw r3, #:lower16:SAVE_R4
    movt r3, #:upper16:SAVE_R4
    ldr r4, [r3]
    mov lr, r5
    ldr r5, [r3, #4]
    mov r0, #0
    movs pc, lr

svc_exit:
    movw r1, #:lower16:RESULT_STATE
    movt r1, #:upper16:RESULT_STATE
    str r0, [r1]
    bkpt #0x0110

kernel_putc:
    movw r1, #:lower16:CURSOR_STATE
    movt r1, #:upper16:CURSOR_STATE
    ldr r2, [r1]

    cmp r0, #10
    beq newline

    movw r3, #:lower16:CRT_BASE
    movt r3, #:upper16:CRT_BASE
    add r3, r3, r2, lsl #1
    strb r0, [r3]
    mov r1, #CRT_ATTR
    strb r1, [r3, #1]
    add r2, r2, #1
    b check_scroll

newline:
    mov r3, r2
newline_mod_loop:
    cmp r3, #CRT_COLS
    blo newline_mod_done
    sub r3, r3, #CRT_COLS
    b newline_mod_loop
newline_mod_done:
    sub r2, r2, r3
    add r2, r2, #CRT_COLS

check_scroll:
    movw r3, #CRT_CELLS
    cmp r2, r3
    blo store_cursor

    movw r3, #:lower16:CRT_BASE
    movt r3, #:upper16:CRT_BASE
    add r1, r3, #(CRT_COLS * 2)
    movw r2, #(CRT_COLS * (CRT_ROWS - 1))

scroll_copy:
    ldrh r0, [r1], #2
    strh r0, [r3], #2
    subs r2, r2, #1
    bne scroll_copy

    mov r0, #0x20
    mov r1, #CRT_ATTR
    mov r2, #CRT_COLS

scroll_clear:
    strb r0, [r3]
    strb r1, [r3, #1]
    add r3, r3, #2
    subs r2, r2, #1
    bne scroll_clear

    movw r2, #(CRT_COLS * (CRT_ROWS - 1))

store_cursor:
    movw r1, #:lower16:CURSOR_STATE
    movt r1, #:upper16:CURSOR_STATE
    str r2, [r1]
    bx r12

vector_undef:     bkpt #0x0101
vector_pabort:    bkpt #0x0103
vector_dabort:    bkpt #0x0104
vector_reserved:  bkpt #0x0105
vector_irq:       bkpt #0x0106
vector_fiq:       bkpt #0x0107
