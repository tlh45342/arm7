.global _start

.equ CRT_BASE,  0x0A000000
.equ CRT_COLS,  80
.equ CRT_ROWS,  25
.equ CRT_SIZE,  CRT_COLS * CRT_ROWS * 2
.equ CURSOR_X,  0x0A001000
.equ CURSOR_Y,  0x0A001004
.equ CURSOR_OFF, 0x0A001000

.equ SVC_INIT_CONSOLE, 1
.equ SVC_PUTS,         2
.equ CURSOR_OFF, 0x0A001000

_start:
vector_table:
    ldr pc, [pc, #24]   @ reset
    ldr pc, [pc, #24]   @ undef
    ldr pc, [pc, #24]   @ svc
    ldr pc, [pc, #24]   @ prefetch abort
    ldr pc, [pc, #24]   @ data abort
    ldr pc, [pc, #24]   @ reserved
    ldr pc, [pc, #24]   @ irq
    ldr pc, [pc, #24]   @ fiq

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

    @ Copy vector table from 0x8000 to 0x00000000
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

    svc #SVC_INIT_CONSOLE

    ldr r0, =hello_msg
    svc #SVC_PUTS

    bkpt #0x1374

default_handler:
    b default_handler

.ltorg

svc_handler:
    sub r12, lr, #4
    ldr r12, [r12]
    bic r12, r12, #0xFF000000

    cmp r12, #SVC_INIT_CONSOLE
    beq svc_init_console

    cmp r12, #SVC_PUTS
    beq svc_puts

    movs pc, lr

svc_init_console:
    ldr r1, =CRT_BASE
    movw r2, #CRT_SIZE
    mov  r3, #' '
    mov  r4, #0x07

clear_loop:
    strb r3, [r1], #1
    strb r4, [r1], #1
    subs r2, r2, #2
    bne clear_loop

    ldr r1, =CURSOR_X
    mov r2, #0
    str r2, [r1]

    ldr r1, =CURSOR_Y
    str r2, [r1]

    movs pc, lr

.ltorg

svc_puts:
    push {r4-r9, lr}

puts_loop:
    ldrb r1, [r0], #1
    cmp r1, #0
    beq puts_done

    push {r0}
    bl console_putc
    pop {r0}

    b puts_loop

puts_done:
    pop {r4-r9, lr}
    movs pc, lr

console_putc:
    ldr r2, =CURSOR_OFF
    ldr r3, [r2]

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

    bx lr

console_newline:
    /*
     * Advance to start of next row.
     * This simple version assumes row width is 160 bytes.
     *
     * while cursor_off is not aligned to 160:
     *     cursor_off += 2
     */
    mov r4, #160

newline_loop:
    cmp r3, r4
    blt newline_set

    sub r3, r3, r4
    b newline_loop

newline_set:
    rsb r5, r3, r4        @ r5 = 160 - remainder

    ldr r3, [r2]          @ reload full cursor offset
    add r3, r3, r5        @ advance to next line
    str r3, [r2]

    bx lr

.ltorg

hello_msg:
    .asciz "HELLO FROM LIBVM BOOTROM\nSECOND LINE\n"

.align 4
.ltorg

.space 4096
stack_top:
