    .arch armv7-a
    .arm
    .text
    .global _start
_start:
    // r10 = 0x00100000
    mov   r10, #0
    movt  r10, #0x0010

    mov   r0, #0x11
    mov   r1, #0x22
    mov   r2, #0x33
    mov   r3, #0x44

    stmia r10!, {r0-r3}      // write 4 words, r10 += 16
    sub   r11, r10, #16
    ldmia r11, {r4-r7}       // read them back

    .word 0xDEADBEEF
	