    .arch armv7-a
    .arm
    .text
    .global _start
_start:
    @ Case A: 0x7FFFFFFF + 1 -> 0x80000000, N=1,Z=0,C=0,V=1
    movw    r0, #0xFFFF
    movt    r0, #0x7FFF
    mov     r1, #1
    adds    r2, r0, r1
    mrs     r10, cpsr

    @ Case B: 0xFFFFFFFF + 1 -> 0x00000000, Z=1,C=1,V=0
    movw    r0, #0xFFFF
    movt    r0, #0xFFFF
    mov     r1, #1
    adds    r3, r0, r1
    mrs     r11, cpsr

    @ ADC consumes previous C=1
    mov     r0, #0
    adc     r6, r0, #0
    mrs     r12, cpsr

halt:
    .word   0xDEADBEEF
