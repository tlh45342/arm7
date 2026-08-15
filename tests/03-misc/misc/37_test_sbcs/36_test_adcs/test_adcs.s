        .text
        .global _start
_start:
        MSR     CPSR_f, #0          // C=0
        MOV     r0, #0xFFFFFFFF
        MOV     r1, #1
        ADCS    r2, r0, r1          // r2=0x00000000, C=1, Z=1, N=0, V=0
        MRS     r10, CPSR
        AND     r10, r10, #(0xF<<28)

        // Carry-in path
        ORR     r9, r9, #0x20000000 // set C=1
        MSR     CPSR_f, r9
        MOV     r3, #0x7FFFFFFF
        MOV     r4, #1
        ADCS    r5, r3, r4          // 0x80000000, C=0, N=1, Z=0, V=1
        MRS     r11, CPSR
        AND     r11, r11, #(0xF<<28)

        NOP
        .word 0xDEADBEEF
