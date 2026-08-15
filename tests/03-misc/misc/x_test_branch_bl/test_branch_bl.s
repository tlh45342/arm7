    .arch armv7-a
    .arm
    .text
    .global _start

add5:
    add     r4, r4, #5
    bx      lr

_start:
    mov     r4, #10
    b       skip
back:
    add     r4, r4, #1           @ should execute once
    b       cont
skip:
    bl      add5                 @ r4 = 15
    b       back
cont:
    @ r4 should be 16 here
halt:
    .word   0xDEADBEEF
	