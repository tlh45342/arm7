    .text
    .global _start

_start:
    // Set known values into r1, r2, r3
    mov r1, #0x11
    mov r2, #0x22
    mov r3, #0x33

    // Push r1, r2, r3 onto the stack
    push {r1, r2, r3}

    // Pop into r4, r5, r6
    pop {r4, r5, r6}

    .word   0xDEADBEEF          // Halt marker
