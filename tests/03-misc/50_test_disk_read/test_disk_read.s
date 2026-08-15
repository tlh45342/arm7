.global _start
_start:
    LDR   r0, =0x0B000000      // disk MMIO base

    // LBA = 19
    MOV   r1, #19
    STR   r1, [r0, #4]

    // Buffer = 0x00010000  (keep in r4 so we don't clobber it)
    LDR   r4, =0x00010000
    STR   r4, [r0, #8]

    // CMD = 1 (start read)  <-- must be offset 0
    MOV   r1, #1
    STR   r1, [r0, #0]

wait_disk:
    LDR   r1, [r0, #12]        // STATUS
    CMP   r1, #0
    BNE   wait_disk

    // Read first byte from buffer and print to UART/CRT
    LDRB  r2, [r4]
    LDR   r3, =0x09000000      // UART/CRT MMIO data port
    STRB  r2, [r3]

    .word 0xDEADBEEF
