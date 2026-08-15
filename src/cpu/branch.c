// src/cpu/branch.c
#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "vm.h"     // for struct VM so we can use e->vm->cpu

// Sign-extend imm24<<2 in one go.
static inline int32_t br_off_imm24(uint32_t instr) {
    int32_t imm = (int32_t)(instr & 0x00FFFFFFu);
    return (imm << 8) >> 6;   // == sign_extend((imm24<<2), 26)
}

static inline CPU *ctx_cpu(exec_ctx_t *e) {
    return &e->vm->cpu;
}

// ---- B (Branch) ----
// If the condition passes, set npc to the branch target. Otherwise leave npc
// as the fall-through (cpu_execute() has already seeded npc = pc + 4).
void handle_b(exec_ctx_t *e) {
    CPU *cpu   = ctx_cpu(e);
    uint32_t A = e->pc;              // address of this instruction
    int32_t off = br_off_imm24(e->instr);

    uint32_t tgt = A + 8u + off;     // ARM A32: PC+8 + signed offset
    cpu->npc     = tgt;
    e->pc        = tgt;              // keep ctx.pc in sync for any tracing
}

// ---- BL (Branch with Link) ----
void handle_bl(exec_ctx_t *e) {
    CPU *cpu   = ctx_cpu(e);
    uint32_t A = e->pc;
    int32_t off = br_off_imm24(e->instr);

    // LR gets the address of the next sequential instruction
    cpu->r[14]  = A + 4u;
    uint32_t tgt = A + 8u + off;
    cpu->npc     = tgt;
    e->pc        = tgt;
}

// ---- BX (Branch and Exchange) ----
// Thumb not modeled yet: just clear bit0 and stay in ARM state.
void handle_bx(exec_ctx_t *e) {
    CPU *cpu   = ctx_cpu(e);
    uint32_t rm  = e->instr & 0xF;
    uint32_t tgt = cpu->r[rm] & ~1u;   // (no T-bit handling yet)

    cpu->npc = tgt;
    e->pc    = tgt;
}

void handle_blx_reg(exec_ctx_t *e) {
    CPU *cpu   = ctx_cpu(e);
    uint32_t A = e->pc;
    uint32_t rm  = e->instr & 0xF;
    uint32_t tgt = cpu->r[rm] & ~1u;

    cpu->r[14] = A + 4u;
    cpu->npc   = tgt;
    e->pc      = tgt;
}

// ---- BLX (Branch with Link, immediate form) ----
// Unconditional (cond=1111 in encoding). ARM-only VM: ignore Thumb request; clear bit0.
// Encoding: 1111 101H imm24
// Target = (A + 8) + sign_extend_26( (imm24 << 2) | (H << 1) )
// LR = A + 4
void handle_blx_imm(exec_ctx_t *e) {
    CPU *cpu   = ctx_cpu(e);
    uint32_t A = e->pc;
    int32_t off = br_off_imm24(e->instr);

    cpu->r[14] = A + 4u;
    uint32_t tgt = (A + 8u + off) & ~1u;   // (no T-bit handling yet)
    cpu->npc     = tgt;
    e->pc        = tgt;
}
