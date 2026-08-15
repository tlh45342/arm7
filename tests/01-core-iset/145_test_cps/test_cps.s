.syntax unified
    .arm
    .global _start
_start:
    @ r6 = 0x0010_0000 (base for stores)
    movw    r6, #0x0000
    movt    r6, #0x0010

    mrs     r0, cpsr           @ snap0: initial CPSR
    str     r0, [r6, #0]

    cpsid   if                 @ disable IRQ+FIQ (sets CPSR.I and CPSR.F)
    mrs     r1, cpsr           @ snap1
    str     r1, [r6, #4]

    cpsie   i                  @ enable IRQ only (clears CPSR.I; F stays set)
    mrs     r2, cpsr           @ snap2
    str     r2, [r6, #8]

    cpsid   a                  @ disable async abort (sets CPSR.A)
    mrs     r3, cpsr           @ snap3
    str     r3, [r6, #12]

    @ (Optional) change mode to SVC in the same instruction:
    cpsie   i, #0x13           @ ensure IRQ enabled and switch to SVC (0b10011)
    mrs     r4, cpsr           @ snap4
    str     r4, [r6, #16]

    bkpt    #0x1234
	