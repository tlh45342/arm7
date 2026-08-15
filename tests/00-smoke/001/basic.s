    .syntax unified
    .arm
    .global _start

/* 001_mem_smoke (ARM mode):
 * - write a word to RAM, read it back
 * - write a byte to RAM, read it back
 * - BKPT so the VM dumps regs (r0/r1/r2/r6) for checks
 */
_start:
    /* Choose a safe RAM address your VM maps. Adjust if needed. */
    ldr     r6, =0x00100000      @ scratch base

    /* WORD round-trip */
    ldr     r0, =0xDEADBEEF      @ test pattern
    str     r0, [r6]             @ [0x00100000] = 0xDEADBEEF
    ldr     r1, [r6]             @ r1 <- 0xDEADBEEF

    /* BYTE round-trip at a nearby offset */
    mov     r2, #0x42            @ 0x42 = 'B'
    strb    r2, [r6, #4]         @ [0x00100004] = 0x42
    ldrb    r2, [r6, #4]         @ r2 <- 0x42

    /* Halt so your harness can assert register values */
    bkpt    #0
