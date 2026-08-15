.arch armv7-a
    .arm
    .text
    .global _start
_start:
    mov     r4, #0x1111
    movt    r4, #0x1111
    mov     r5, #0x2222
    movt    r5, #0x2222
    mov     r6, #0x3333
    movt    r6, #0x3333
    mov     r7, #0x4444
    movt    r7, #0x4444

    mov     r8, sp               @ save original SP snapshot
    push    {r4-r7, lr}
    mov     r4, #0               @ clobber
    mov     r5, #0
    mov     r6, #0
    mov     r7, #0
    pop     {r4-r7, lr}
    sub     r9, sp, r8           @ r9 = delta (should be 0 after push+pop)

halt:
    .word   0xDEADBEEF
	