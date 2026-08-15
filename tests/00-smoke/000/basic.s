// basic.s
    .syntax unified
    .arch armv7-a
    .text
    .global _start

_start:
    mov     r3, #1
    mov     r4, #2

    // Halt (your emulator treats this as a stop)
    bkpt    #0x1234
	