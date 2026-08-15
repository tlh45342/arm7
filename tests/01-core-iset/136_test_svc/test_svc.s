    .syntax unified
    .cpu cortex-a15
    .arch armv7-a
    .text
    .global _start

/* Where we’ll drop a “I ran” marker from the SVC handler */
.equ MARK_ADDR, 0x00100000

/* Simple vector table we’ll copy to 0x00000000.
   We use the classic "ldr pc, [pc, #off]" entries so it’s position-independent.
   PC at the time of each entry is (entry_addr + 8), so #24 lands on the table of addresses. */
.align 4
vector_table:
    ldr pc, [pc, #24]   /* 0x00: Reset      -> v_reset        */
    ldr pc, [pc, #24]   /* 0x04: Undef      -> v_undef        */
    ldr pc, [pc, #24]   /* 0x08: SVC        -> v_svc          */
    ldr pc, [pc, #24]   /* 0x0C: PrefetchAb -> v_pabt         */
    ldr pc, [pc, #24]   /* 0x10: DataAbt    -> v_dabt         */
    ldr pc, [pc, #24]   /* 0x14: (reserved) -> v_reserved     */
    ldr pc, [pc, #24]   /* 0x18: IRQ        -> v_irq          */
    ldr pc, [pc, #24]   /* 0x1C: FIQ        -> v_fiq          */

    /* Table of target addresses (same order as above) */
v_reset:     .word reset
v_undef:     .word undef_handler
v_svc:       .word svc_handler
v_pabt:      .word default_handler
v_dabt:      .word default_handler
v_reserved:  .word default_handler
v_irq:       .word default_handler
v_fiq:       .word default_handler

/* Entry point at 0x8000 */
_start:
reset:
    /* Copy 32 bytes (8 entries) + 32 bytes (address table) = 64 bytes is enough */
    ldr   r0, =vector_table
    ldr   r1, =0x00000000
    mov   r2, #64            /* bytes to copy */

1:  ldr   r3, [r0], #4
    str   r3, [r1], #4
    subs  r2, r2, #4
    bne   1b

    /* Make sure the CPU sees the new vector contents */
    dsb   sy
    isb

    /* Clear marker */
    ldr   r0, =MARK_ADDR
    mov   r1, #0
    str   r1, [r0]

    /* Trigger SVC with an immediate (0x42 just for fun) */
    svc   #0x42

    /* If we returned correctly, drop 0x12345678 so we know we resumed */
    ldr   r0, =MARK_ADDR
    ldr   r1, =0x12345678
    str   r1, [r0]

    // Halt execution
    bkpt    0x1234
	
/* ------- Exception handlers ------- */

undef_handler:
default_handler:
    /* Park here if something unexpected hits */
    b     default_handler

/* SVC handler:
   - We’re in SVC mode. LR_svc points to the *next* instr after SVC.
   - Read the SVC opcode at (LR - 4) if you care about imm24.
   - Return with MOVS pc, lr to restore CPSR from SPSR_svc and resume *after* SVC. */
svc_handler:
    /* Optional: read the SVC immediate (imm24) by fetching the instruction at (LR - 4) */
    sub     r0, lr, #4
    ldr     r1, [r0]          /* r1 = SVC instruction (0xEF000000 | imm24) */
    /* imm24 = r1 & 0x00FFFFFF if you care */

    /* Drop a marker so we can see the handler ran */
    ldr     r2, =MARK_ADDR
    ldr     r3, =0xDEADBEEF
    str     r3, [r2]

    /* Return from exception: restore CPSR from SPSR_svc and continue after SVC */
    movs    pc, lr             /* (equivalently: subs pc, lr, #0) */
