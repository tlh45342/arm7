    .syntax unified
    .arch armv7-a
    .arm

    .global _start
_start:
    /* results base: r6 = 0x0010_0000 */
    movw    r6, #0x0000
    movt    r6, #0x0010

    /* save CPSR before */
    mrs     r12, cpsr

    /* --- 1) x = 0x00000000 => clz = 32 --- */
    mov     r1, #0
    clz     r0, r1
    str     r0, [r6, #0]          /* 0x00000020 */

    /* --- 2) x = 0x00000001 => clz = 31 --- */
    mov     r1, #1
    clz     r0, r1
    str     r0, [r6, #4]          /* 0x0000001F */

    /* --- 3) x = 0x80000000 => clz = 0 --- */
    movw    r1, #0x0000
    movt    r1, #0x8000
    clz     r0, r1
    str     r0, [r6, #8]          /* 0x00000000 */

    /* --- 4) x = 0x00F00000 => clz = 8 --- */
    movw    r1, #0x0000
    movt    r1, #0x00F0
    clz     r0, r1
    str     r0, [r6, #12]         /* 0x00000008 */

    /* --- 5) x = 0x00008000 => clz = 16 --- */
    movw    r1, #0x8000
    movt    r1, #0x0000
    clz     r0, r1
    str     r0, [r6, #16]         /* 0x00000010 */

    /* --- 6) x = 0x12345678 => clz = 3 --- */
    movw    r1, #0x5678
    movt    r1, #0x1234
    clz     r0, r1
    str     r0, [r6, #20]         /* 0x00000003 */

    /* save CPSR after and store XOR to prove flags unchanged */
    mrs     r11, cpsr
    eor     r10, r12, r11
    str     r10, [r6, #24]        /* expect 0x00000000 */

    /* CLZ 0xFFFFFFFF -> 0 */
    mvn     r1, #0
    clz     r0, r1
    str     r0, [r6, #28]

    /* halt for harness */
    bkpt    #0x1234
	