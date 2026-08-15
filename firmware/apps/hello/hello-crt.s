.global _start

.equ CRT_BASE, 0x0A000000
.equ CRT_ATTR, 0x07

_start:
    movw    r0, #0x0000
    movt    r0, #0x0A00

    mov     r2, #CRT_ATTR

    mov     r1, #'H'
    strb    r1, [r0]
    strb    r2, [r0, #1]
    add     r0, r0, #2

    mov     r1, #'E'
    strb    r1, [r0]
    strb    r2, [r0, #1]
    add     r0, r0, #2

    mov     r1, #'L'
    strb    r1, [r0]
    strb    r2, [r0, #1]
    add     r0, r0, #2

    mov     r1, #'L'
    strb    r1, [r0]
    strb    r2, [r0, #1]
    add     r0, r0, #2

    mov     r1, #'O'
    strb    r1, [r0]
    strb    r2, [r0, #1]
    add     r0, r0, #2

    mov     r1, #' '
    strb    r1, [r0]
    strb    r2, [r0, #1]
    add     r0, r0, #2

    mov     r1, #'W'
    strb    r1, [r0]
    strb    r2, [r0, #1]
    add     r0, r0, #2

    mov     r1, #'O'
    strb    r1, [r0]
    strb    r2, [r0, #1]
    add     r0, r0, #2

    mov     r1, #'R'
    strb    r1, [r0]
    strb    r2, [r0, #1]
    add     r0, r0, #2

    mov     r1, #'L'
    strb    r1, [r0]
    strb    r2, [r0, #1]
    add     r0, r0, #2

    mov     r1, #'D'
    strb    r1, [r0]
    strb    r2, [r0, #1]

    bkpt    #0x1374
