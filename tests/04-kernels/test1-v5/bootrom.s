.global _start

.equ CRT_BASE,          0x0A000000
.equ CRT_COLS,          80
.equ CRT_ROWS,          25
.equ CRT_SIZE,          CRT_COLS * CRT_ROWS * 2
.equ CRT_ROW_BYTES,     CRT_COLS * 2
.equ CRT_LAST_ROW_OFF,  CRT_SIZE - CRT_ROW_BYTES
.equ CURSOR_OFF,        0x0A001000

.equ SVC_CONSOLE_INIT,  1
.equ SVC_CONSOLE_PUTS,  2
.equ SVC_CONSOLE_PUTC,  3

_start:
vector_table:
    ldr pc, [pc, #24]
    ldr pc, [pc, #24]
    ldr pc, [pc, #24]
    ldr pc, [pc, #24]
    ldr pc, [pc, #24]
    ldr pc, [pc, #24]
    ldr pc, [pc, #24]
    ldr pc, [pc, #24]

    .word reset
    .word default_handler
    .word svc_handler
    .word default_handler
    .word default_handler
    .word default_handler
    .word default_handler
    .word default_handler

reset:
    ldr sp, =stack_top

    ldr r0, =vector_table
    mov r1, #0
    mov r2, #64

copy_vectors:
    ldr r3, [r0], #4
    str r3, [r1], #4
    subs r2, r2, #4
    bne copy_vectors

    dsb
    isb

    svc #SVC_CONSOLE_INIT

    ldr r0, =hello_msg
    svc #SVC_CONSOLE_PUTS

    bkpt #0x1374

default_handler:
    b default_handler

.ltorg

svc_handler:
    sub r12, lr, #4
    ldr r12, [r12]
    bic r12, r12, #0xFF000000

    cmp r12, #SVC_CONSOLE_INIT
    beq svc_console_init

    cmp r12, #SVC_CONSOLE_PUTS
    beq svc_console_puts

    cmp r12, #SVC_CONSOLE_PUTC
    beq svc_console_putc

    movs pc, lr

svc_console_init:
    ldr r1, =CRT_BASE
    movw r2, #CRT_SIZE
    mov  r3, #' '
    mov  r4, #0x07

clear_loop:
    strb r3, [r1], #1
    strb r4, [r1], #1
    subs r2, r2, #2
    bne clear_loop

    ldr r1, =CURSOR_OFF
    mov r2, #0
    str r2, [r1]

    movs pc, lr

.ltorg

svc_console_puts:
    mov r6, lr

puts_loop:
    ldrb r1, [r0], #1
    cmp r1, #0
    beq puts_done

    mov r7, r0

    ldr lr, =puts_after_putc
    b console_putc

puts_after_putc:
    mov r0, r7
    b puts_loop

puts_done:
    mov lr, r6
    movs pc, lr

svc_console_putc:
    mov r1, r0
    b console_putc

console_putc:
    mov r10, lr

    ldr r2, =CURSOR_OFF
    ldr r3, [r2]

    movw r4, #CRT_SIZE
    cmp r3, r4
    blt console_check_char

    bl console_scroll
    ldr r2, =CURSOR_OFF
    ldr r3, [r2]

console_check_char:
    cmp r1, #10
    beq console_newline

console_normal_char:
    ldr r8, =CRT_BASE
    add r8, r8, r3

    strb r1, [r8]

    mov r9, #0x07
    strb r9, [r8, #1]

    add r3, r3, #2
    str r3, [r2]

    bx r10

console_newline:
    mov r4, #CRT_ROW_BYTES

newline_loop:
    cmp r3, r4
    blt newline_set

    sub r3, r3, r4
    b newline_loop

newline_set:
    rsb r5, r3, r4

    ldr r3, [r2]
    add r3, r3, r5
    str r3, [r2]

    movw r4, #CRT_SIZE
    cmp r3, r4
    blt newline_done

    bl console_scroll

newline_done:
    bx r10

console_scroll:
    /*
     * Scroll screen up by one row:
     * copy rows 1-24 to rows 0-23,
     * clear last row,
     * set cursor to start of last row.
     */

    ldr r0, =CRT_BASE
    add r1, r0, #CRT_ROW_BYTES

    movw r2, #CRT_LAST_ROW_OFF

scroll_copy_loop:
    ldrb r3, [r1], #1
    strb r3, [r0], #1
    subs r2, r2, #1
    bne scroll_copy_loop

    ldr r0, =CRT_BASE
    movw r1, #CRT_LAST_ROW_OFF
    add r0, r0, r1

    mov r2, #CRT_ROW_BYTES
    mov r3, #' '
    mov r4, #0x07

scroll_clear_last_row:
    strb r3, [r0], #1
    strb r4, [r0], #1
    subs r2, r2, #2
    bne scroll_clear_last_row

    ldr r0, =CURSOR_OFF
    movw r1, #CRT_LAST_ROW_OFF
    str r1, [r0]

    bx lr

.ltorg

hello_msg:
    .asciz "LINE 01\nLINE 02\nLINE 03\nLINE 04\nLINE 05\nLINE 06\nLINE 07\nLINE 08\nLINE 09\nLINE 10\nLINE 11\nLINE 12\nLINE 13\nLINE 14\nLINE 15\nLINE 16\nLINE 17\nLINE 18\nLINE 19\nLINE 20\nLINE 21\nLINE 22\nLINE 23\nLINE 24\nLINE 25\nLINE 26\nLINE 27\nLINE 28\nLINE 29\nLINE 30"

.align 4
.ltorg

.space 4096
stack_top:
