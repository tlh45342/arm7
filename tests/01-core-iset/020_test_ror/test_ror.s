.syntax unified
.arm
.global _start
_start:
    @ r6 = 0x0010_0000 (base for stores)
    movw    r6, #0
    movt    r6, #0x0010

    @ r0 = 0x80000001 (MSB+LSB set)
    movw    r0, #1
    movt    r0, #0x8000

    @ Case A: ROR #1 -> r1; capture flags
    movs    r1, r0, ror #1
    mrs     r7, cpsr
    str     r1, [r6, #0]
    str     r7, [r6, #4]

    @ Case B: ROR #8 -> r2; capture flags
    movs    r2, r0, ror #8
    mrs     r7, cpsr
    str     r2, [r6, #8]
    str     r7, [r6, #12]

    @ (Optional later: RRX via 'rrxs r3, r0' after priming C)

    bkpt    #0x1234
