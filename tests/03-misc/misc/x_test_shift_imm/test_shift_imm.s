 .arch armv7-a
    .arm
    .text
    .global _start
_start:
    mov     r0, #1
    movt    r0, #0x8000          @ r0 = 0x80000001

    movs    r1, r0, lsl #1       @ r1=0x00000002, C=bit31(original)=1
    mrs     r8, cpsr

    movs    r2, r0, lsr #1       @ r2=0x40000000, C=bit0(original)=1
    mrs     r9, cpsr

    movs    r3, r0, asr #31      @ r3=0xFFFFFFFF, C=bit30(original)=0
    mrs     r10, cpsr

    movs    r4, r0, ror #1       @ r4=0xC0000000, C=bit0(original)=1
    mrs     r11, cpsr

    @ Emulate LSL #32 (not encodable): result=0, C=bit0(original)
    movs    r5, r0, lsl #31      @ r5 = r0<<31; C = bit1(original)
    movs    r5, r5, lsl #1       @ r5 = 0;     C = bit31(prev r5) = bit0(original)
    mrs     r12, cpsr

halt:
    .word   0xDEADBEEF
	