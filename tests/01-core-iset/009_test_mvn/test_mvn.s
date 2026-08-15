    .syntax unified
    .cpu cortex-a8
    .global _start
    .text

_start:
    // Case 9.1: MVN immediate
    mvn     r0, #0                    // r0 = 0xFFFFFFFF
    mov     r2, #0x91
    nop                               // EXPECT: r0=0xFFFFFFFF; N=1 Z=0

    // Case 9.2: MVN register
    mov     r1, #0xAA
    mvn     r3, r1                    // r3 = ~0xAA
    mov     r2, #0x92
    nop                               // EXPECT: r3=~0xAA

    // Case 9.3: MVNS (imm, rotate==0) — C preserved
    msr     cpsr_f, #0x20000000       // C=1
    mvns    r4, #0                    // result=0xFFFFFFFF, C=1
    mov     r2, #0x93
    nop                               // EXPECT: r4=0xFFFFFFFF; N=1 Z=0 C=1

    // Case 9.4: MVNS (reg, LSR #1)
    mov     r0, #1
    mvns    r5, r0, lsr #1            // Rm>>1=0 => ~0=0xFFFFFFFF; carry=old bit0 (1)
    mov     r2, #0x94
    nop                               // EXPECT: r5=0xFFFFFFFF; N=1 Z=0 C=1

    // Case 9.5: MVNS (reg, RRX)
    msr     cpsr_f, #0x20000000       // C=1
    mov     r0, #1
    mvns    r6, r0, rrx               // RRX(1,C=1)=0x80000000 => ~ = 0x7FFFFFFF; carry=1
    mov     r2, #0x95
    nop                               // EXPECT: r6=0x7FFFFFFF; N=0 Z=0 C=1

    // Case 9.6: MVNS (reg, LSL by register) — bit4==1 encoding
    mov     r0, #1
    mov     r1, #1
    mvns    r7, r0, lsl r1            // result=~2=0xFFFFFFFD; carry=bit31 of r0 (0)
    mov     r2, #0x96
    nop                               // EXPECT: r7=0xFFFFFFFD; N=1 Z=0 C=0

    // Case 9.7: MVNS (reg, LSL by register with amount=0) — carry preserved
    msr     cpsr_f, #0x20000000       // C=1
    mov     r0, #0x55
    mov     r1, #0
    mvns    r8, r0, lsl r1            // result=~r0; carry preserved (1)
    mov     r2, #0x97
    nop                               // EXPECT: r8=~0x55; N=1 Z=0 C=1

    // Halt
    bkpt    #0               // halt
	