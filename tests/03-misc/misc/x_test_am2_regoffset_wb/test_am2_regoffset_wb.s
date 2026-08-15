// Exercises AM2 register-offset loads with immediate shifts and write-back.
// Uses only encodings you already support:
//  - STR (imm, no-wb) to seed memory
//  - LDR (register offset, pre-index with !)

        .global _start
        .text
_start:
        // r6 = 0x00100100 (base)
        movw    r6, #0x0100
        movt    r6, #0x0010

        // Seed mem[base + (3<<2)] = 0x33333333
        mov     r7, #3                  // offset = 12
        movw    r2, #0x3333
        movt    r2, #0x3333
        str     r2, [r6, #12]

        // LDR with reg offset pre/wb: [r6, r7, LSL #2]!
        // r8 should get 0x33333333, r6 becomes base+12
        ldr     r8, [r6, r7, LSL #2]!

        // Seed mem[(base+12) - (2<<3)] = mem[base - 4] = 0x44444444
        mov     r7, #2                  // offset = 16
        movw    r2, #0x4444
        movt    r2, #0x4444
        str     r2, [r6, #-16]

        // LDR with reg offset pre/wb: [r6, -r7, LSL #3]!
        // r9 should get 0x44444444, r6 becomes base-4
        ldr     r9, [r6, -r7, LSL #3]!

        .word   0xDEADBEEF
		