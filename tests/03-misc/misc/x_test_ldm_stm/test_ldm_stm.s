    .arch armv7-a
    .arm
    .data
buf:
    .space  32
    .text
    .global _start
_start:
    ldr     r0, =buf             @ base
    mov     r4, #0x11
    mov     r5, #0x22
    mov     r6, #0x33

    stmia   r0!, {r4-r6}         @ buf[0..2] = 11,22,33 ; r0 += 12
    sub     r1, r0, #12          @ r1 = &buf

    ldmia   r1, {r7-r9}          @ r7=0x11, r8=0x22, r9=0x33
    mov     r10, r0              @ r10 should be &buf+12 after writeback

halt:
    .word   0xDEADBEEF
    .ltorg
	