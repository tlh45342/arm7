.global _start
.section .text

_start:
    ldr     sp, =0x1FFFFFFC   @ set stack pointer
    bl      main              @ call directly into C main
    .word   0xDEADBEEF
	