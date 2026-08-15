.syntax unified
    .arch armv7-a
    .text
    .global _start
    .arm

/* r6: scratch base (VM usually sets this; we just use it) */

_start:
    // Standard two words you’ve been using up front (tick your early decoders)
    tst     r0, #0          // 0xE3006000
    cmp     r0, #0x10       // 0xE3406010

    // Make a handy source: r1 = 0x12345678
    movw    r1, #0x5678
    movt    r1, #0x1234

    // r2 = 0xAAAAAAAA (for an obvious xor pattern)
    movw    r2, #0xAAAA
    movt    r2, #0xAAAA

    // --- Case A: EOR (register) ---
    // r0 = r1 ^ r2  => 0x12345678 ^ 0xAAAAAAAA = 0xB89EFCD2
    eor     r0, r1, r2
    str     r0, [r6, #0]

    // --- Case B: EORS with shifted register (tests Z=1, C=1) ---
    // r3 = 0, r4 = 1; (r4 LSR #1) = 0; carry-out = bit0 of r4 = 1
    // r5 = r3 ^ (r4 LSR #1) = 0; Z=1, N=0; C=1; V unchanged
    mov     r3, #0
    mov     r4, #1
    eors    r5, r3, r4, lsr #1
    mrs     r7, cpsr
    str     r7, [r6, #4]        // expect CPSR ..ZC.. = 0x60000000
    str     r5, [r6, #8]        // expect 0

    // --- Case C: EOR (immediate, small) ---
    // r8 = r1 ^ 0x80  => 0x12345678 ^ 0x00000080 = 0x123456F8
    eor     r8, r1, #0x80
    str     r8, [r6, #12]

    // --- Case D: EOR (immediate, high bit) ---
    // r9 = r1 ^ 0x80000000 => 0x92345678
    eor     r9, r1, #0x80000000
    str     r9, [r6, #16]

    // --- Case E: EORS with ASR #1 (tests N=1, C=1) ---
    // r4 = 0x80000001; (ASR #1) = 0xC0000000; carry-out = bit0 (1)
    // r10 = 0 ^ 0xC0000000 = 0xC0000000 => N=1, Z=0, C=1
    movw    r4, #0x0001
    movt    r4, #0x8000
    mov     r3, #0
    eors    r10, r3, r4, asr #1
    mrs     r11, cpsr
    str     r10, [r6, #20]      // expect 0xC0000000
    str     r11, [r6, #24]      // expect 0xA0000000 (N=1, C=1)

    // Halt
    bkpt    0x1234
	