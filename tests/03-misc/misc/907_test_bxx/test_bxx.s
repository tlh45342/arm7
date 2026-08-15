.syntax unified
    .arch armv7-a
    .text
    .global _start

    .macro TAG_TO_MEM tag, off
        mov   r1, #\tag
        str   r1, [r6, #\off]
    .endm

_start:
    // r6 = 0x00100000 (scratch area)
    movw  r6, #0
    movt  r6, #0x100

    // --- 1) BEQ: Z=1 -> taken
    mov   r0, #0
    cmp   r0, #0                // Z=1
    beq   1f
    TAG_TO_MEM 0xEE, 0
    b     2f
1:  TAG_TO_MEM 0x01, 0
2:

    // --- 2) BNE: Z=0 -> taken
    mov   r0, #0
    cmp   r0, #1                // Z=0
    bne   1f
    TAG_TO_MEM 0xEE, 4
    b     2f
1:  TAG_TO_MEM 0x02, 4
2:

    // --- 3) BHS/BCS: C=1 -> taken (0 >= 0)
    mov   r0, #0
    cmp   r0, #0                // C=1
    bhs   1f                    // (=BCS)
    TAG_TO_MEM 0xEE, 8
    b     2f
1:  TAG_TO_MEM 0x03, 8
2:

    // --- 4) BLO/BCC: C=0 -> taken (0 < 1)
    mov   r0, #0
    cmp   r0, #1                // C=0
    blo   1f                    // (=BCC)
    TAG_TO_MEM 0xEE, 12
    b     2f
1:  TAG_TO_MEM 0x04, 12
2:

    // --- 5) BMI: N=1 -> taken (make negative)
    mov   r0, #0
    subs  r0, r0, #1            // r0=0xFFFFFFFF, N=1
    bmi   1f
    TAG_TO_MEM 0xEE, 16
    b     2f
1:  TAG_TO_MEM 0x05, 16
2:

    // --- 6) BPL: N=0 -> taken (zero result)
    mov   r0, #0
    subs  r0, r0, #0            // r0=0, N=0
    bpl   1f
    TAG_TO_MEM 0xEE, 20
    b     2f
1:  TAG_TO_MEM 0x06, 20
2:

    // --- 7) BVS: V=1 -> taken (overflow: 0x7FFFFFFF + 1)
    mov   r0, #0x7F
    orr   r0, r0, #0x7F00
    orr   r0, r0, #0x7F0000
    orr   r0, r0, #0x800000     // r0 = 0x7FFFFFFF
    adds  r0, r0, #1            // V=1
    bvs   1f
    TAG_TO_MEM 0xEE, 24
    b     2f
1:  TAG_TO_MEM 0x07, 24
2:

    // --- 8) BVC: V=0 -> taken (simple add)
    mov   r0, #1
    adds  r0, r0, #1            // V=0
    bvc   1f
    TAG_TO_MEM 0xEE, 28
    b     2f
1:  TAG_TO_MEM 0x08, 28
2:

    // --- 9) BGE: N==V -> taken (1 >= 0 signed)
    mov   r1, #1
    mov   r2, #0
    cmp   r1, r2                // N=0,V=0 => N==V
    bge   1f
    TAG_TO_MEM 0xEE, 32
    b     2f
1:  TAG_TO_MEM 0x09, 32
2:

    // --- 10) BLT: N!=V -> taken (-2 < 0 signed)
    mvn   r1, #1                // r1 = 0xFFFFFFFE (-2)
    mov   r2, #0
    cmp   r1, r2                // N=1,V=0 => N!=V
    blt   1f
    TAG_TO_MEM 0xEE, 36
    b     2f
1:  TAG_TO_MEM 0x0A, 36
2:

    // --- 11) BGT: Z==0 && N==V -> taken (2 > 1 signed)
    mov   r1, #2
    mov   r2, #1
    cmp   r1, r2                // Z=0, N=0,V=0
    bgt   1f
    TAG_TO_MEM 0xEE, 40
    b     2f
1:  TAG_TO_MEM 0x0B, 40
2:

    // --- 12) BLE: Z==1 || N!=V -> taken (1 <= 1 via Z=1)
    mov   r1, #1
    mov   r2, #1
    cmp   r1, r2                // Z=1
    ble   1f
    TAG_TO_MEM 0xEE, 44
    b     2f
1:  TAG_TO_MEM 0x0C, 44
2:

    // --- 13) BHI: C==1 && Z==0 -> taken (2 > 1 unsigned)
    mov   r1, #2
    mov   r2, #1
    cmp   r1, r2                // C=1, Z=0
    bhi   1f
    TAG_TO_MEM 0xEE, 48
    b     2f
1:  TAG_TO_MEM 0x0D, 48
2:

    // --- 14) BLS: C==0 || Z==1 -> taken (1 <= 2 unsigned via C=0)
    mov   r1, #1
    mov   r2, #2
    cmp   r1, r2                // C=0
    bls   1f
    TAG_TO_MEM 0xEE, 52
    b     2f
1:  TAG_TO_MEM 0x0E, 52
2:

    bkpt    #0x1234
	