    .arch armv7-a
    .arm
    .text
    .global _start

_start:
    mov     r0, #1
    ldr     r1, =target
    blx     r1                 @ requires BLX(reg) implemented
    b       halt

target:
    add     r0, r0, #7
    bx      lr

halt:
    .word   0xDEADBEEF
    .ltorg
	