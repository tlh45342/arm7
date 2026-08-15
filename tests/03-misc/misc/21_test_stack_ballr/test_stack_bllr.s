  .text
    .global _start

_start:
    // Load address of _end into r0
    ldr r0, =_end

    // Load 8MB (0x800000) into sp, then add r0 to get sp = _end + 8MB
    ldr sp, =0x00800000        // 8MB
    add sp, r0, sp             // sp = _end + 8MB

    // Set up some test values
    mov r0, #0x42
    add r1, r0, #1

    // Call a dummy subroutine that pushes/pops
    bl test_func

    // Sentinel to detect successful return
    .word 0xDEADBEEF

// --------------------------------
// Dummy subroutine to test stack
// --------------------------------
test_func:
    push {r0, r1}      // Save r0, r1
    add r1, r1, #1      // Do something
    pop {r0, r1}        // Restore
    bx lr

// --------------------------------
// Mark the end of the binary
// --------------------------------
_end:
