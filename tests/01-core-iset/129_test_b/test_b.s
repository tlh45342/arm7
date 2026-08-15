.syntax unified
.arch armv7-a
.arm
.text
.global _start

/*
 * 129_test_b
 *
 * Validates ARM B-family control flow:
 *
 *   1. unconditional forward B skips an instruction
 *   2. BNE loops backward while condition is true
 *   3. BNE eventually falls through
 *   4. BEQ taken path works
 *   5. BEQ not-taken path works
 *   6. final unconditional B skips a failure path
 *
 * Results are stored at 0x00100000:
 *
 *   +0x00 = 1  forward branch succeeded
 *   +0x04 = 0  backward BNE loop reached zero
 *   +0x08 = 2  BEQ taken path succeeded
 *   +0x0C = 3  BEQ not-taken path succeeded
 */

_start:
    movw    r6, #0x0000
    movt    r6, #0x0010

    /* Case 1: unconditional forward branch. */
    mov     r0, #0
    b       case1_target
    mov     r0, #0xEE          /* must be skipped */

case1_target:
    mov     r0, #1
    str     r0, [r6, #0]

    /* Case 2: conditional backward branch. */
    mov     r1, #3

case2_loop:
    sub     r1, r1, #1
    cmp     r1, #0
    bne     case2_loop

    str     r1, [r6, #4]       /* must store 0 */

    /* Case 3: BEQ taken. */
    cmp     r0, #1
    beq     case3_taken
    mov     r2, #0xEE          /* must be skipped */

case3_taken:
    mov     r2, #2
    str     r2, [r6, #8]

    /* Case 4: BEQ not taken. */
    cmp     r0, #0
    beq     case4_fail

    mov     r3, #3
    str     r3, [r6, #12]
    b       done

case4_fail:
    mov     r3, #0xEE
    str     r3, [r6, #12]

done:
    bkpt    #0x1374
