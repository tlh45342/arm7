        .text
        .global _start
_start:
        MSR     CPSR_f, #0          // C=0 => borrow-in = 1
        MOV     r0, #0x00000000
        MOV     r1, #0x00000000
        SBCS    r2, r0, r1          // 0 - 0 - 1 => 0xFFFFFFFF, C=0, N=1, Z=0, V=0
        MRS     r10, CPSR
        AND     r10, r10, #(0xF<<28)

        ORR     r9, r9, #0x20000000 // set C=1 => borrow-in=0
        MSR     CPSR_f, r9
        MOV     r3, #0x80000000
        MOV     r4, #1
        SBCS    r5, r3, r4          // 0x7FFFFFFF, C=1, N=0, Z=0, V=1
        MRS     r11, CPSR
        AND     r11, r11, #(0xF<<28)

        NOP
        .word 0xDEADBEEF
