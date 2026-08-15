    .arch armv7-a
    .arm
    .data
memarea:
    .word   0x11223344
    .word   0x55667788
    .word   0xDEADC0DE
    .text
    .global _start
_start:
    ldr     r0, =memarea         @ base
    mov     r1, #0xA5A5
    movt    r1, #0x5A5A          @ r1 = 0x5A5AA5A5

    str     r1, [r0, #0]         @ mem[0] = r1
    ldr     r2, [r0, #0]         @ r2 should read back 0x5A5AA5A5

    add     r3, r0, #4
    str     r2, [r3], #4         @ post-index: store at mem[1], r3 += 4
    ldr     r4, [r0, #4]         @ r4 = mem[1] = r2

    add     r5, r0, #8
    str     r4, [r5, #-4]!       @ pre-index writeback: (r5 = r5-4), store at mem[1] again
    ldr     r6, [r0, #4]         @ r6 = mem[1] (same as r4)

halt:
    .word   0xDEADBEEF
    .ltorg
	