 .syntax unified
    .arm
    .global _start
_start:
    /* r6 = 0x0010_0000 (result base) */
    movw    r6, #0x0000
    movt    r6, #0x0010

    /* --- A: clear ALL 32 bits (lsb=0, width=32) --- */
    movw    r0, #0x5678
    movt    r0, #0x1234            @ r0 = 0x12345678
    bfc     r0, #0, #32            @ r0 = 0x00000000
    str     r0, [r6, #0]           @ expect 0x00000000

    /* --- B: clear middle byte (bits 8..15) --- */
    movw    r1, #0xABCD
    movt    r1, #0x1234            @ r1 = 0x1234ABCD
    bfc     r1, #8, #8             @ r1 = 0x123400CD
    str     r1, [r6, #4]           @ expect 0x123400CD

    /* --- C: clear low nibble’s high 4 bits (bits 4..7) --- */
    movw    r2, #0x4321
    movt    r2, #0x8765            @ r2 = 0x87654321
    bfc     r2, #4, #4             @ r2 = 0x87654301
    str     r2, [r6, #8]           @ expect 0x87654301

    /* --- D: clear an 8-bit field that crosses a byte boundary (bits 20..27) --- */
    movw    r3, #0xF0F0
    movt    r3, #0xF0F0            @ r3 = 0xF0F0F0F0
    bfc     r3, #20, #8            @ clears b2[7:4] and b3[3:0] → r3 = 0xF000F0F0
    str     r3, [r6, #12]          @ expect 0xF000F0F0

    /* Halt so harness can read back */
    bkpt    #0x1234
	