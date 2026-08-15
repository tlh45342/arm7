    .syntax unified
    .arch armv7-a
    .text
    .global _start

_start:
    nop

    // Halt
    bkpt    #0x1234
	