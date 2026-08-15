    .syntax unified
    .cpu cortex-a8
    .global _start
    .text

_start:
    // Case 8.1: MOV immediate
    mov     r0, #0x42                 // r0 = 0x42
    mov     r2, #0x81                 // case marker
    nop                               // EXPECT: r0=0x42; NZ=00; C unchanged

    // Case 8.2: MOV register
    mov     r1, r0                    // r1 = 0x42
    mov     r2, #0x82
    nop                               // EXPECT: r1=0x42

    // Case 8.3: MOV with immediate shift
    mov     r3, r0, lsl #1            // r3 = 0x84
    mov     r2, #0x83
    nop                               // EXPECT: r3=0x84

    // Case 8.4: MOVS (imm, rotate==0) — C preserved
    msr     cpsr_f, #0                // C=0
    movs    r4, #1
    mov     r2, #0x84
    nop                               // EXPECT: r4=1; N=0 Z=0 C=0

    msr     cpsr_f, #0x20000000       // C=1
    movs    r4, #1
    mov     r2, #0x85
    nop                               // EXPECT: r4=1; N=0 Z=0 C=1

    // Case 8.5: MOVS (reg, LSL #31)
    // r0 currently 0x42; make r0 = 0x80000000
    mov     r0, #0x80
    mov     r0, r0, lsl #24           // r0 = 0x80000000
    movs    r5, r0, lsl #31           // result=0, carry=bit(32-31)=bit1 of r0 (0)
    mov     r2, #0x86
    nop                               // EXPECT: r5=0; N=0 Z=1 C=0

    // Case 8.6: MOVS (reg, LSR #1)
    mov     r0, #1
    movs    r6, r0, lsr #1            // result=0, carry=old bit0 (1)
    mov     r2, #0x87
    nop                               // EXPECT: r6=0; N=0 Z=1 C=1

    // Case 8.7: MOVS (reg, RRX)
    msr     cpsr_f, #0x20000000       // C=1
    mov     r0, #1
    movs    r7, r0, rrx               // result=0x80000000, carry=old bit0 (1)
    mov     r2, #0x88
    nop                               // EXPECT: r7=0x80000000; N=1 Z=0 C=1

    // Case 8.8: MOVS (reg, LSL by register) — bit4==1 encoding
    mov     r0, #1
    mov     r1, #1
    movs    r8, r0, lsl r1            // result=2, carry=bit31 of r0 (0)
    mov     r2, #0x89
    nop                               // EXPECT: r8=2; N=0 Z=0 C=0

    // Case 8.9: MOVS (reg, LSL by register with amount=0) — carry preserved
    msr     cpsr_f, #0x20000000       // C=1
    mov     r0, #0x55
    mov     r1, #0                    // shift amount in Rs = 0
    movs    r9, r0, lsl r1            // result=r0, carry preserved (1)
    mov     r2, #0x8A
    nop                               // EXPECT: r9=0x55; N=0 Z=0 C=1

    // Halt
    bkpt    #0               // halt
	