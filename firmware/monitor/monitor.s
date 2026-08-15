.syntax unified
.arch armv7-a
.arm

/*
 * ARM7 Monitor v0.0.4
 *
 * Load address : 0x00020000
 *
 * Adds a tiny command parser on top of the proven keyboard/CRT path.
 *
 * Commands:
 *   help   - list commands
 *   about  - show monitor version
 *   clear  - clear CRT and redraw monitor banner
 *   halt   - stop at BKPT
 *
 * Keyboard MMIO:
 *   0x0A001000 KEY_STATUS bit0 READY
 *   0x0A001004 KEY_DATA   low byte; read pops FIFO
 *
 * Still intentionally absent:
 *   - IRQ-driven input
 *   - RTC command
 *   - disk command
 *   - scrolling
 *   - SVC ABI
 */

.equ CRT_BASE,        0x0A000000
.equ CRT_COLS,        80
.equ CRT_ROWS,        25
.equ CRT_CELL_SIZE,   2
.equ CRT_ROW_STRIDE,  (CRT_COLS * CRT_CELL_SIZE)
.equ CRT_END,         (CRT_BASE + (CRT_ROWS * CRT_ROW_STRIDE))
.equ CRT_ATTR,        0x07

.equ KEY_STATUS,      0x0A001000
.equ KEY_READY,       0x00000001

.equ RTC_BASE,        0xF0002000
.equ RTC_HOUR_OFF,    0x1C
.equ RTC_MIN_OFF,     0x20
.equ RTC_SEC_OFF,     0x24

.equ INPUT_MAX,       63

.section .text, "ax"
.align 2
.global _start

_start:
    /*
     * Persistent register use:
     *   r5  CRT attribute
     *   r6  cursor cell address
     *   r7  current row base
     *   r8  keyboard MMIO base
     *   r9  input buffer base
     *   r10 input length
     */
    mov     r5, #CRT_ATTR

    movw    r8, #:lower16:KEY_STATUS
    movt    r8, #:upper16:KEY_STATUS

    movw    r9, #:lower16:input_buffer
    movt    r9, #:upper16:input_buffer

    /* Keep BIOS and BOOT banners; monitor starts on row 2. */
    movw    r7, #:lower16:(CRT_BASE + (2 * CRT_ROW_STRIDE))
    movt    r7, #:upper16:(CRT_BASE + (2 * CRT_ROW_STRIDE))

    movw    r0, #:lower16:monitor_banner
    movt    r0, #:upper16:monitor_banner
    mov     r1, r7
    bl      print_string_at

    add     r7, r7, #CRT_ROW_STRIDE
    b       print_prompt

/* --------------------------------------------------------------
 * Prompt and line input.
 * -------------------------------------------------------------- */
print_prompt:
    mov     r10, #0
    mov     r6, r7

    mov     r0, #'>'
    strb    r0, [r6]
    strb    r5, [r6, #1]
    add     r6, r6, #CRT_CELL_SIZE

    mov     r0, #' '
    strb    r0, [r6]
    strb    r5, [r6, #1]
    add     r6, r6, #CRT_CELL_SIZE

keyboard_poll:
    ldr     r0, [r8]
    tst     r0, #KEY_READY
    beq     keyboard_poll

    ldr     r2, [r8, #4]
    and     r2, r2, #0xFF

    cmp     r2, #13
    beq     handle_enter

    cmp     r2, #8
    beq     handle_backspace

    cmp     r2, #32
    blo     keyboard_poll
    cmp     r2, #126
    bhi     keyboard_poll

    cmp     r10, #INPUT_MAX
    bhs     keyboard_poll

    /* Do not let input run past the visible row. */
    add     r3, r7, #CRT_ROW_STRIDE
    cmp     r6, r3
    bhs     keyboard_poll

    strb    r2, [r9, r10]
    add     r10, r10, #1

    strb    r2, [r6]
    strb    r5, [r6, #1]
    add     r6, r6, #CRT_CELL_SIZE
    b       keyboard_poll

handle_backspace:
    cmp     r10, #0
    beq     keyboard_poll

    sub     r10, r10, #1
    sub     r6, r6, #CRT_CELL_SIZE

    mov     r0, #' '
    strb    r0, [r6]
    strb    r5, [r6, #1]
    b       keyboard_poll

handle_enter:
    mov     r0, #0
    strb    r0, [r9, r10]

    add     r7, r7, #CRT_ROW_STRIDE
    bl      normalize_row

    cmp     r10, #0
    beq     print_prompt

    b       dispatch_command

/* --------------------------------------------------------------
 * Command dispatcher.
 * -------------------------------------------------------------- */
dispatch_command:
    mov     r0, r9
    movw    r1, #:lower16:cmd_help
    movt    r1, #:upper16:cmd_help
    bl      streq
    cmp     r0, #0
    beq     do_help

    mov     r0, r9
    movw    r1, #:lower16:cmd_about
    movt    r1, #:upper16:cmd_about
    bl      streq
    cmp     r0, #0
    beq     do_about

    mov     r0, r9
    movw    r1, #:lower16:cmd_clear
    movt    r1, #:upper16:cmd_clear
    bl      streq
    cmp     r0, #0
    beq     do_clear

    mov     r0, r9
    movw    r1, #:lower16:cmd_time
    movt    r1, #:upper16:cmd_time
    bl      streq
    cmp     r0, #0
    beq     do_time

    mov     r0, r9
    movw    r1, #:lower16:cmd_halt
    movt    r1, #:upper16:cmd_halt
    bl      streq
    cmp     r0, #0
    beq     do_halt

    movw    r0, #:lower16:msg_unknown
    movt    r0, #:upper16:msg_unknown
    mov     r1, r7
    bl      print_string_at

    add     r7, r7, #CRT_ROW_STRIDE
    bl      normalize_row
    b       print_prompt

do_help:
    movw    r0, #:lower16:msg_help1
    movt    r0, #:upper16:msg_help1
    mov     r1, r7
    bl      print_string_at

    add     r7, r7, #CRT_ROW_STRIDE
    bl      normalize_row
    movw    r0, #:lower16:msg_help2
    movt    r0, #:upper16:msg_help2
    mov     r1, r7
    bl      print_string_at

    add     r7, r7, #CRT_ROW_STRIDE
    bl      normalize_row
    movw    r0, #:lower16:msg_help3
    movt    r0, #:upper16:msg_help3
    mov     r1, r7
    bl      print_string_at

    add     r7, r7, #CRT_ROW_STRIDE
    bl      normalize_row
    movw    r0, #:lower16:msg_help4
    movt    r0, #:upper16:msg_help4
    mov     r1, r7
    bl      print_string_at

    add     r7, r7, #CRT_ROW_STRIDE
    bl      normalize_row

    movw    r0, #:lower16:msg_help5
    movt    r0, #:upper16:msg_help5
    mov     r1, r7
    bl      print_string_at

    add     r7, r7, #CRT_ROW_STRIDE
    bl      normalize_row
    b       print_prompt

do_about:
    movw    r0, #:lower16:monitor_banner
    movt    r0, #:upper16:monitor_banner
    mov     r1, r7
    bl      print_string_at

    add     r7, r7, #CRT_ROW_STRIDE
    bl      normalize_row
    b       print_prompt

do_clear:
    bl      clear_screen

    /* After clear, redraw monitor banner at row 0. */
    movw    r7, #:lower16:CRT_BASE
    movt    r7, #:upper16:CRT_BASE

    movw    r0, #:lower16:monitor_banner
    movt    r0, #:upper16:monitor_banner
    mov     r1, r7
    bl      print_string_at

    add     r7, r7, #CRT_ROW_STRIDE
    b       print_prompt

do_time:
    movw    r4, #:lower16:RTC_BASE
    movt    r4, #:upper16:RTC_BASE

    ldr     r0, [r4, #RTC_HOUR_OFF]
    mov     r1, r7
    bl      print_two_digits

    mov     r0, #':'
    add     r1, r7, #4
    bl      put_cell

    ldr     r0, [r4, #RTC_MIN_OFF]
    add     r1, r7, #6
    bl      print_two_digits

    mov     r0, #':'
    add     r1, r7, #10
    bl      put_cell

    ldr     r0, [r4, #RTC_SEC_OFF]
    add     r1, r7, #12
    bl      print_two_digits

    add     r7, r7, #CRT_ROW_STRIDE
    bl      normalize_row
    b       print_prompt

print_two_digits:
    mov     r2, #0
1:
    cmp     r0, #10
    blo     2f
    sub     r0, r0, #10
    add     r2, r2, #1
    b       1b
2:
    add     r2, r2, #'0'
    strb    r2, [r1]
    strb    r5, [r1, #1]
    add     r0, r0, #'0'
    strb    r0, [r1, #2]
    strb    r5, [r1, #3]
    bx      lr

put_cell:
    strb    r0, [r1]
    strb    r5, [r1, #1]
    bx      lr

do_halt:
    movw    r0, #:lower16:msg_halt
    movt    r0, #:upper16:msg_halt
    mov     r1, r7
    bl      print_string_at

    bkpt    #0x3003

/* --------------------------------------------------------------
 * streq
 *   r0 = left string
 *   r1 = right string
 * Returns r0 = 0 if equal, 1 if different.
 * No stack required.
 * -------------------------------------------------------------- */
streq:
streq_loop:
    ldrb    r2, [r0], #1
    ldrb    r3, [r1], #1
    cmp     r2, r3
    bne     streq_no
    cmp     r2, #0
    bne     streq_loop

    mov     r0, #0
    bx      lr

streq_no:
    mov     r0, #1
    bx      lr

/* --------------------------------------------------------------
 * print_string_at
 *   r0 = zero-terminated string
 *   r1 = CRT cell address
 * Clobbers r2,r3.
 * -------------------------------------------------------------- */
print_string_at:
print_string_loop:
    ldrb    r2, [r0], #1
    cmp     r2, #0
    beq     print_string_done

    strb    r2, [r1]
    strb    r5, [r1, #1]
    add     r1, r1, #CRT_CELL_SIZE
    b       print_string_loop

print_string_done:
    bx      lr

/* --------------------------------------------------------------
 * clear_screen
 * Clears all 80x25 character cells.
 * -------------------------------------------------------------- */
clear_screen:
    movw    r0, #:lower16:CRT_BASE
    movt    r0, #:upper16:CRT_BASE
    movw    r1, #:lower16:CRT_END
    movt    r1, #:upper16:CRT_END
    mov     r2, #' '

clear_loop:
    cmp     r0, r1
    bhs     clear_done

    strb    r2, [r0]
    strb    r5, [r0, #1]
    add     r0, r0, #CRT_CELL_SIZE
    b       clear_loop

clear_done:
    bx      lr

/* --------------------------------------------------------------
 * normalize_row
 * Wraps output back to row 0 if we reach the end of CRT.
 * Scrolling comes later.
 * -------------------------------------------------------------- */
normalize_row:
    movw    r0, #:lower16:CRT_END
    movt    r0, #:upper16:CRT_END
    cmp     r7, r0
    blo     normalize_done

    movw    r7, #:lower16:CRT_BASE
    movt    r7, #:upper16:CRT_BASE

normalize_done:
    bx      lr

.align 2
monitor_banner:
    .asciz "ARM7 Monitor v0.0.4"

cmd_help:
    .asciz "help"
cmd_about:
    .asciz "about"
cmd_clear:
    .asciz "clear"
cmd_time:
    .asciz "time"
cmd_halt:
    .asciz "halt"

msg_unknown:
    .asciz "Unknown command"

msg_help1:
    .asciz "help   - show commands"
msg_help2:
    .asciz "about  - monitor version"
msg_help3:
    .asciz "clear  - clear screen"
msg_help4:
    .asciz "time   - show RTC time"
msg_help5:
    .asciz "halt   - stop CPU"

msg_halt:
    .asciz "CPU halted"

.section .data
.align 2
input_buffer:
    .space 64
