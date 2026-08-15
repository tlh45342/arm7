// test_add.s
    .syntax unified
    .arch armv7-a
    .text
    .global _start
_start:
    // r0 = 0
    mov     r0, #0

    // --- ADD (immediate) ---
    // r1 = r0 + #5 => 5
    add     r1, r0, #5

    // r2 = r1 + #0x120 (tests rotate/imm handling if you support it)
    // For now, keep it small so it encodes without rotate:
    add     r2, r1, #7             // expect 12

    // --- ADD (register) ---
    mov     r3, #10
    add     r4, r2, r3              // r4 = 12 + 10 = 22

    // Wraparound sanity (optional)
    mvn     r5, #0                  // r5 = 0xFFFFFFFF
    add     r6, r5, #1              // r6 = 0

    // Halt
    bkpt    #0x1234
