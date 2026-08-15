    .global _start

_start:
    MOV     r0, #'X'            @ ASCII 'X' = 0x58
    LDR     r1, =0x0A000000     @ CRT_BASE
    STRB    r0, [r1]            @ Write byte to CRT[0][0]

    .word   0xDEADBEEF          @ Mark end for VM halt
