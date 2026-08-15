    .syntax unified
    .arm
    .global _start

_start:
    // r6 := 0x0010_0000 (result buffer base)
    movw    r6, #0x0000
    movt    r6, #0x0010

    // breadcrumb before BKPT
    mov     r1, #0x11
    str     r1, [r6, #0]

    // BKPT with 16-bit immediate (encodes to 0xE1323074 for #0x1234)
    bkpt    #0x1234
    // If your assembler complains in ARM mode, you can use the raw word:
    // .word   0xE1323074

    // should NOT run if BKPT halts
    mov     r1, #0x22
    str     r1, [r6, #4]

    // Halt (your emulator treats this as a stop)
    bkpt    #0x1234
