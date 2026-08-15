.syntax unified
    .arm
    .global _start

/* Load at 0x8000 (your runner sets PC anyway) */
_start:
    /* r4 = &DATA */
    adr     r4, DATA

    /* ---- LDRD (immediate) ----
       Rt must be even and not r15; Rt+1 must not be r15: use r0,r1 OK */
    ldrd    r0, r1, [r4]

    /* r6 = 0x00100000 (dest #1) */
    movw    r6, #:lower16:0x00100000
    movt    r6, #:upper16:0x00100000

    /* STRD r0,r1 -> [r6] */
    strd    r0, r1, [r6]

    /* ---- LDRD (register offset) ----
       r5 = +8, so we read the NEXT two words (just to exercise reg-offset form).
       Here the next two words mirror the first pair for easy checking. */
    mov     r5, #8
    ldrd    r2, r3, [r4, r5]

    /* r7 = 0x00100010 (dest #2) */
    movw    r7, #:lower16:0x00100010
    movt    r7, #:upper16:0x00100010

    /* STRD r2,r3 -> [r7] */
    strd    r2, r3, [r7]

    /* Halt (your emulator’s easter egg) */
    .word   0xDEADBEEF

    .align 3              /* keep data nicely aligned */
DATA:
    .word   0x11223344
    .word   0x55667788
    /* duplicate pair to make the reg-offset LDRD read legal/obvious */
    .word   0x11223344
    .word   0x55667788
	