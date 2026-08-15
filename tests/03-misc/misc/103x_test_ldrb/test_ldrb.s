.section .text
.global _start
_start:
    LDR  r0, =string        @ r0 -> start of string
    LDRB r1, [r0], #1       @ load first byte into r1, then r0 += 1
    .word 0xdeadbeef        @ halt

.section .data
string:
    .asciz "XYZ\n"
