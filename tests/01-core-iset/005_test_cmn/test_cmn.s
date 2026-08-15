// test_cmn.s
.syntax unified
.arch armv7-a
.text
.global _start

_start:
    // r6 = 0x0010_0000 (result base)
    movw    r6, #0
    movt    r6, #0x0010

    // zero some regs
    mov     r0, #0
    mov     r1, #0
    mov     r2, #0
    mov     r3, #0
    mov     r4, #0
    mov     r5, #0
    mov     r7, #0
    mov     r12, #0

// ---- Case 1: 0 + 0 = 0 -> Z=1 --------------------------------------------
    mov     r3, #0
    mov     r4, #0
    cmn     r3, r4
    mrs     r7, cpsr
    str     r7, [r6, #0]          // expect 0x40000000

// ---- Case 2: 0xFFFFFFFF + 1 = 0 -> Z=1, C=1 -------------------------------
    mvn     r3, r0                 // r3 = 0xFFFFFFFF
    mov     r4, #1
    cmn     r3, r4
    mrs     r7, cpsr
    str     r7, [r6, #4]          // expect 0x60000000

// ---- Case 3: 0x7FFFFFFF + 1 = 0x80000000 -> N=1, V=1 ----------------------
    movw    r3, #0xFFFF
    movt    r3, #0x7FFF            // r3 = 0x7FFFFFFF
    mov     r4, #1
    cmn     r3, r4
    mrs     r7, cpsr
    str     r7, [r6, #8]          // expect 0x90000000

// ---- Case 4: 0x80000000 + 0x80000000 = 0 -> Z=1, C=1, V=1 -----------------
    movw    r3, #0
    movt    r3, #0x8000            // r3 = 0x80000000
    mov     r4, r3
    cmn     r3, r4
    mrs     r7, cpsr
    str     r7, [r6, #12]         // expect 0x70000000

// ---- Case 5: reg-shift op2: 5 + (1<<1) = 7 -> no flags --------------------
    mov     r3, #5
    mov     r4, #1
    cmn     r3, r4, lsl #1
    mrs     r7, cpsr
    str     r7, [r6, #16]         // expect 0x00000000


// ---- Case 6: RRX with C-in=1: 5 + 0x80000000 -> N=1, others 0 ----
    subs    r12, r0, r0        // sets C=1 (and Z=1), leaves N,V=0
    mov     r3, #5
    mov     r4, #0
    cmn     r3, r4, rrx        // use explicit RRX, not ROR #0
    mrs     r7, cpsr
    str     r7, [r6, #20]      // expect 0x80000000
    // Halt
    bkpt    #0x1234
	