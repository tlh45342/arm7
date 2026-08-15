.global _start

_start:
    /* result base = 0x00100000 */
    movw    r6, #0x0000
    movt    r6, #0x0010

    /*
     * Case 1:
     * BX to address in r0.
     */
    ldr     r0, =bx_target_1
    bx      r0

bx_fail_1:
    mov     r1, #0x11
    str     r1, [r6, #0]
    bkpt    #0xBAD

bx_target_1:
    mov     r1, #0xAA
    str     r1, [r6, #0]

    /*
     * Case 2:
     * BX lr as subroutine return.
     */
    bl      sub_bx_return
    str     r2, [r6, #4]

    /*
     * Case 3:
     * BXEQ not taken.
     */
    mov     r3, #1
    mov     r4, #2
    cmp     r3, r4
    ldr     r5, =bx_fail_3
    bxeq    r5

    mov     r7, #0x33
    str     r7, [r6, #8]
    b       after_case_3

bx_fail_3:
    mov     r7, #0x77
    str     r7, [r6, #8]
    bkpt    #0xBAD

after_case_3:
    /*
     * Case 4:
     * BXEQ taken.
     */
    mov     r3, #5
    mov     r4, #5
    cmp     r3, r4
    ldr     r5, =bx_target_4
    bxeq    r5

bx_fail_4:
    mov     r8, #0x44
    str     r8, [r6, #12]
    bkpt    #0xBAD

bx_target_4:
    mov     r8, #0x55
    str     r8, [r6, #12]

    /*
     * Case 5:
     * BX should not alter CPSR flags.
     */
    cmp     r8, r8
    ldr     r9, =bx_target_5
    bx      r9

bx_fail_5:
    mov     r10, #0x66
    str     r10, [r6, #16]
    bkpt    #0xBAD

bx_target_5:
    mrs     r10, cpsr
    str     r10, [r6, #16]

    bkpt    #0x1374


sub_bx_return:
    mov     r2, #0x22
    bx      lr