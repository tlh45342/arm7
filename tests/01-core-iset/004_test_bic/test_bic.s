    .text
    .global _start
_start:
    // r0 baseline used in all tests
    MOV     r0, #0xF0

    // Clear NZCVQ so we know C starts at 0
    MSR     CPSR_f, #0

    // -------- I=1 (imm), rot == 0  --------
    // res = 0xF0 & ~0xF0 = 0x00000000
    // Flags: Z=1, N=0, C (unchanged) = 0
    BICS    r1, r0, #0xF0
    MRS     r10, CPSR
    AND     r10, r10, #(0xF << 28)     // keep NZCV only

    // -------- I=1 (imm), rot != 0  --------
    // op2 = 0x80000000 -> shifter C = bit31(op2) = 1
    // res = 0xF0 & ~0x80000000 = 0x000000F0
    // Flags: N=0, Z=0, C=1
    BICS    r2, r0, #0x80000000
    MRS     r11, CPSR
    AND     r11, r11, #(0xF << 28)

    // -------- I=0 (reg), LSL #31 --------
    // Rm = 1, LSL #31 -> op2 = 0x80000000, shifter C = (Rm>>(32-31))&1 = 0
    // Rn = 0x80000001 so result clears bit31 => 0x00000001
    // Flags: N=0, Z=0, C=0
    MOV     r3, #1
    ORR     r3, r3, #0x80000000        // r3 = 0x80000001 (Rn)
    MOV     r4, #1                     // Rm
    BICS    r5, r3, r4, LSL #31
    MRS     r12, CPSR
    AND     r12, r12, #(0xF << 28)

    // -------- I=0 (reg), RRX path (ROR #0) --------
    // Set C=1, then RRX with Rm=0 -> op2=(C<<31)|(Rm>>1)=0x80000000, shC=Rm bit0=0
    // res = 0xF0 & ~0x80000000 = 0x000000F0
    // Flags: N=0, Z=0, C=0
    MRS     r9, CPSR
    ORR     r9, r9, #0x20000000        // set C bit
    MSR     CPSR_f, r9
    MOV     r6, #0
    BICS    r7, r0, r6, ROR #0         // RRX
    MRS     r13, CPSR
    AND     r13, r13, #(0xF << 28)

    // Final register MOV
    MOV     r14, r14

    // Halt
    bkpt    #0x1234
		