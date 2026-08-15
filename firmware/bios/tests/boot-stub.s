.syntax unified
.arch armv7-a
.arm
.text
.global _start

.equ CRT_BASE,       0x0A000000
.equ CRT_COLS,       80
.equ CRT_CELL_SIZE,  2
.equ CRT_ROW_STRIDE, (CRT_COLS * CRT_CELL_SIZE)
.equ CRT_ROW1,       (CRT_BASE + CRT_ROW_STRIDE)
.equ CRT_ATTR,       0x07

_start:
    /* r4 = row 1, column 0 */
    movw    r4, #:lower16:CRT_ROW1
    movt    r4, #:upper16:CRT_ROW1

    movw    r0, #:lower16:msg
    movt    r0, #:upper16:msg

    mov     r5, #CRT_ATTR

1:
    ldrb    r1, [r0], #1
    cmp     r1, #0
    beq     2f

    strb    r1, [r4]
    strb    r5, [r4, #1]
    add     r4, r4, #CRT_CELL_SIZE
    b       1b

2:
    bkpt    #0xB007

.align 2
msg:
    .asciz "BOOT HANDOFF OK"
