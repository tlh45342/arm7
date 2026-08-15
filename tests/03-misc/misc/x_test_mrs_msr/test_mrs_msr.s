    .arch armv7-a
    .arm
    .text
    .global _start
_start:
    mrs     r0, cpsr
    @ Example (future): msr cpsr_f, r0
halt:
    .word   0xDEADBEEF