// test_adc.s
    .syntax unified
    .arch armv7-a
    .text
    .global _start

_start:
    // r6 = 0x0010_0000 (result base)
    movw    r6, #0
    movt    r6, #0x0001

    // zero some regs
    mov     r0, #0
    mov     r1, #0
    mov     r2, #0
    mov     r3, #0
    mov     r4, #0
    mov     r5, #0
    mov     r7, #0

// ---------------- Case 1: Cin=1, 1 + 2 + Cin = 4, flags clear ----------------
    // Set C=1 via SUBS r12, r0, r0  (0 - 0 -> C=1, Z=1)
    subs    r12, r0, r0

    mov     r3, #1
    mov     r4, #2
    adcs    r5, r3, r4          // r5 = 1 + 2 + Cin(1) = 4
    mrs     r7, cpsr
    str     r5, [r6, #0]
    str     r7, [r6, #4]

// ---------------- Case 2: Cin=0, 0xFFFFFFFF + 1 = 0, C=1, Z=1 ----------------
    // Set C=0 via SUBS r12, r0, #1  (0 - 1 -> borrow -> C=0)
    subs    r12, r0, #1

    mvn     r3, r0              // r3 = 0xFFFFFFFF
    mov     r4, #1
    adcs    r5, r3, r4          // Cin=0 here
    mrs     r7, cpsr
    str     r5, [r6, #8]
    str     r7, [r6, #12]

// --------------- Case 3: Cin=1, 0x7FFFFFFF + 0 + 1 = 0x80000000, V=1 ---------
    // Set C=1 again
    subs    r12, r0, r0

    movw    r3, #0xFFFF
    movt    r3, #0x7FFF         // r3 = 0x7FFFFFFF
    mov     r4, #0
    adcs    r5, r3, r4          // + Cin(1)
    mrs     r7, cpsr
    str     r5, [r6, #16]
    str     r7, [r6, #20]

// ---- Case 4: Cin=0, 0x80000000 + 0x80000000 = 0, C=1, Z=1, V=1 --------------
    // Set C=0
    subs    r12, r0, #1

    movw    r3, #0
    movt    r3, #0x8000         // r3 = 0x80000000
    movw    r4, #0
    movt    r4, #0x8000         // r4 = 0x80000000
    adcs    r5, r3, r4          // Cin=0
    mrs     r7, cpsr
    str     r5, [r6, #24]
    str     r7, [r6, #28]

// -------- Case 5: Cin=1, register-shift op2: 5 + (1<<1) + 1 = 8 --------------
    // Set C=1
    subs    r12, r0, r0

    mov     r3, #5
    mov     r4, #1
    adcs    r5, r3, r4, lsl #1  // 5 + 2 + Cin(1) = 8
    mrs     r7, cpsr
    str     r5, [r6, #32]
    str     r7, [r6, #36]

    // Halt (your emulator treats this as a stop)
    bkpt    #0x1234
	