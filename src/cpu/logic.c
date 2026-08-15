// src/cpu/logic.c — exec_ctx_t* + VM-owned CPU (no e->cpu)
// - All CPU state is accessed via e->vm->cpu
// - Local CPSR helpers operate on e->vm->cpu.cpsr
// - Keeps dp_operand2() (implemented in operand.c), so no inline globals leak

#include <stdint.h>
#include <stdbool.h>

#include "cond.h"
#include "logic.h"
#include "operand.h"    // dp_operand2()
#include "cpu.h"        // for CPU definition (cpsr/spsr)
#include "vm.h"         // VM, CPU inside VM

// ---- CPSR bit masks (standard ARM A-profile) ----
#ifndef CPSR_N
#define CPSR_N (1u << 31)
#endif
#ifndef CPSR_Z
#define CPSR_Z (1u << 30)
#endif
#ifndef CPSR_C
#define CPSR_C (1u << 29)
#endif
#ifndef CPSR_V
#define CPSR_V (1u << 28)
#endif

// ---- Local CPSR helpers that operate on e->vm->cpu ----
static inline uint32_t cpsr_get_C_e(const exec_ctx_t *e) {
    const CPU *cpu = &e->vm->cpu;
    return (cpu->cpsr & CPSR_C) ? 1u : 0u;
}

static inline void cpsr_set_NZ_e(exec_ctx_t *e, uint32_t res) {
    CPU *cpu = &e->vm->cpu;
    uint32_t p = cpu->cpsr;
    p &= ~(CPSR_N | CPSR_Z);
    if (res == 0) p |= CPSR_Z;
    if (res & 0x80000000u) p |= CPSR_N;
    cpu->cpsr = p;
}

static inline void cpsr_set_C_from_e(exec_ctx_t *e, uint32_t c) {
    CPU *cpu = &e->vm->cpu;
    if (c & 1u) cpu->cpsr |= CPSR_C;
    else        cpu->cpsr &= ~CPSR_C;
}

static inline void cpsr_set_V_e(exec_ctx_t *e, int v) {
    CPU *cpu = &e->vm->cpu;
    if (v) cpu->cpsr |= CPSR_V;
    else   cpu->cpsr &= ~CPSR_V;
}

// ---- Exception return helper ----
// Implements architectural data-processing exception return:
// any DP instruction with S=1 and Rd=15 (e.g. MOVS pc, lr).
static inline void exception_return_ctx(exec_ctx_t *e, uint32_t new_pc)
{
    CPU *cpu = &e->vm->cpu;

    // Restore CPSR from SPSR (single-bank SPSR model for now).
    cpu->cpsr = cpu->spsr;

    // Clear T bit (bit0); we stay in ARM state in this VM.
    uint32_t tgt = new_pc & ~1u;

    // Drive next PC via npc; cpu_execute() will commit it.
    cpu->npc = tgt;
    e->pc    = tgt;
}

// ------------------------------- ORR -------------------------------
void handle_orr(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !arm_condition_holds(e, cond)) return;

    uint32_t S  = (instr >> 20) & 1u;
    uint32_t Rn = (instr >> 16) & 0xFu;
    uint32_t Rd = (instr >> 12) & 0xFu;

    uint32_t sh_carry = cpsr_get_C_e(e);
    uint32_t op2 = dp_operand2(instr, &sh_carry);
    uint32_t res = cpu->r[Rn] | op2;

    if (Rd == 15u) {
        if (S) {
            exception_return_ctx(e, res);
        } else {
            uint32_t tgt = res & ~1u;
            cpu->npc = tgt;
            e->pc    = tgt;
        }
        return;
    }

    cpu->r[Rd] = res;
    if (S) { cpsr_set_NZ_e(e, res); cpsr_set_C_from_e(e, sh_carry); }
}

// ------------------------------- BIC -------------------------------
void handle_bic(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !arm_condition_holds(e, cond)) return;

    uint32_t S  = (instr >> 20) & 1u;
    uint32_t Rn = (instr >> 16) & 0xFu;
    uint32_t Rd = (instr >> 12) & 0xFu;

    uint32_t sh_carry = cpsr_get_C_e(e);
    uint32_t op2 = dp_operand2(instr, &sh_carry);
    uint32_t res = cpu->r[Rn] & ~op2;

    if (Rd == 15u) {
        if (S) {
            exception_return_ctx(e, res);
        } else {
            uint32_t tgt = res & ~1u;
            cpu->npc = tgt;
            e->pc    = tgt;
        }
        return;
    }

    cpu->r[Rd] = res;
    if (S) { cpsr_set_NZ_e(e, res); cpsr_set_C_from_e(e, sh_carry); }
}

// ------------------------------- AND imm/reg fast paths -------------------------------
void handle_and_imm_dp(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !arm_condition_holds(e, cond)) return;

    uint32_t S  = (instr >> 20) & 1u;
    uint32_t Rn = (instr >> 16) & 0xFu;
    uint32_t Rd = (instr >> 12) & 0xFu;

    uint32_t sh_carry = cpsr_get_C_e(e);
    uint32_t op2 = dp_operand2(instr, &sh_carry);
    uint32_t res = cpu->r[Rn] & op2;

    if (Rd == 15u) {
        if (S) {
            exception_return_ctx(e, res);
        } else {
            uint32_t tgt = res & ~1u;
            cpu->npc = tgt;
            e->pc    = tgt;
        }
        return;
    }

    cpu->r[Rd] = res;
    if (S) { cpsr_set_NZ_e(e, res); cpsr_set_C_from_e(e, sh_carry); }
}

void handle_and_reg_simple(exec_ctx_t *e) { handle_and_imm_dp(e); }

// ------------------------------- TST imm/reg -------------------------------
void handle_tst_imm(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !arm_condition_holds(e, cond)) return;

    uint32_t Rn = (instr >> 16) & 0xFu;

    uint32_t sh_carry = cpsr_get_C_e(e);
    uint32_t op2 = dp_operand2(instr, &sh_carry);
    uint32_t res = cpu->r[Rn] & op2;

    cpsr_set_NZ_e(e, res);
    cpsr_set_C_from_e(e, sh_carry);
}

void handle_tst_reg(exec_ctx_t *e) { handle_tst_imm(e); }

// ------------------------------- CMP reg/imm -------------------------------
void handle_cmp_reg(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !arm_condition_holds(e, cond)) return;

    uint32_t Rn = (instr >> 16) & 0xFu;

    uint32_t sh_carry = cpsr_get_C_e(e);
    uint32_t op2 = dp_operand2(instr, &sh_carry);
    uint32_t a   = cpu->r[Rn];

    uint32_t res = a - op2;
    cpsr_set_NZ_e(e, res);
    cpsr_set_C_from_e(e, a >= op2); // NOT borrow
    int overflow = ((a ^ op2) & (a ^ res) & 0x80000000u) != 0;
    cpsr_set_V_e(e, overflow);
}

void handle_cmp_imm(exec_ctx_t *e) { handle_cmp_reg(e); }

// ------------------------------- MOV (DP form) -------------------------------
void handle_mov(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !arm_condition_holds(e, cond)) return;

    uint32_t S  = (instr >> 20) & 1u;
    uint32_t Rd = (instr >> 12) & 0xFu;

    uint32_t sh_carry = cpsr_get_C_e(e);
    uint32_t res = dp_operand2(instr, &sh_carry);

    if (Rd == 15u) {
        if (S) {
            // Exception-return path (e.g., MOVS pc, lr)
            exception_return_ctx(e, res);
        } else {
            uint32_t tgt = res & ~1u;
            cpu->npc = tgt;
            e->pc    = tgt;
        }
        return;
    }

    cpu->r[Rd] = res;
    if (S) { cpsr_set_NZ_e(e, res); cpsr_set_C_from_e(e, sh_carry); }
}

// ------------------------------- RSB (imm fast path) -------------------------------

void handle_rsb_imm(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !arm_condition_holds(e, cond)) return;

    uint32_t S  = (instr >> 20) & 1u;
    uint32_t Rn = (instr >> 16) & 0xFu;
    uint32_t Rd = (instr >> 12) & 0xFu;

    uint32_t sh_carry = cpsr_get_C_e(e);
    uint32_t op2 = dp_operand2(instr, &sh_carry);
    uint32_t a   = cpu->r[Rn];

    uint32_t res = op2 - a;

    if (Rd == 15u) {
        if (S) {
            // Exception-return path (e.g., SUBS/RSBS pc, lr, #imm)
            exception_return_ctx(e, res);
        } else {
            uint32_t tgt = res & ~1u;
            cpu->npc = tgt;
            e->pc    = tgt;
        }
        return;
    }

    cpu->r[Rd] = res;
    if (S) {
        cpsr_set_NZ_e(e, res);
        cpsr_set_C_from_e(e, op2 >= a); // NOT borrow
        int overflow = ((op2 ^ a) & (op2 ^ res) & 0x80000000u) != 0;
        cpsr_set_V_e(e, overflow);
    }
}

// ------------------------------- CMN (imm fast path) -------------------------------

void handle_cmn_imm(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !arm_condition_holds(e, cond)) return;

    uint32_t Rn = (instr >> 16) & 0xFu;

    uint32_t sh_carry = cpsr_get_C_e(e);
    uint32_t op2 = dp_operand2(instr, &sh_carry);
    uint32_t a   = cpu->r[Rn];

    uint32_t res = a + op2;
    cpsr_set_NZ_e(e, res);
    cpsr_set_C_from_e(e, res < a); // carry out
    int overflow = (~(a ^ op2) & (a ^ res) & 0x80000000u) != 0;
    cpsr_set_V_e(e, overflow);
}

// ------------------------------- MOVW / MOVT / MOV(imm) -------------------------------

void handle_movw(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !arm_condition_holds(e, cond)) return;

    uint32_t Rd    = (instr >> 12) & 0xFu;
    uint32_t imm4  = (instr >> 16) & 0xFu;
    uint32_t imm12 =  instr        & 0xFFFu;
    uint32_t res   = (imm4 << 12) | imm12;

    if (Rd == 15u) {
        uint32_t tgt = res & ~1u;
        cpu->npc = tgt;
        e->pc    = tgt;
        return;
    }
    cpu->r[Rd] = res;
}

void handle_movt(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !arm_condition_holds(e, cond)) return;

    uint32_t Rd    = (instr >> 12) & 0xFu;
    uint32_t imm4  = (instr >> 16) & 0xFu;
    uint32_t imm12 =  instr        & 0xFFFu;
    uint32_t imm16 = (imm4 << 12) | imm12;

    uint32_t base  = cpu->r[Rd] & 0x0000FFFFu;
    uint32_t res   = base | (imm16 << 16);

    if (Rd == 15u) {
        uint32_t tgt = res & ~1u;
        cpu->npc = tgt;
        e->pc    = tgt;
        return;
    }
    cpu->r[Rd] = res;
}

void handle_mov_imm(exec_ctx_t *e) { handle_mov(e); }

// Rd==PC is UNPREDICTABLE for these; keep VM robust by ignoring the write.
#define IGNORE_IF_PC(rd) do { if ((rd) == 15u) return; } while(0)

// ---------------- Byte/half reversals ----------------

void handle_rev(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;
    IGNORE_IF_PC(Rd);
    uint32_t x = cpu->r[Rm];
#if defined(__GNUC__) || defined(__clang__)
    cpu->r[Rd] = __builtin_bswap32(x);
#else
    cpu->r[Rd] = (x >> 24)
               | ((x >> 8)  & 0x0000FF00u)
               | ((x << 8)  & 0x00FF0000u)
               | (x << 24);
#endif
}

// -----------------------------------------------------------------------------------

void handle_rev16(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;
    IGNORE_IF_PC(Rd);
    uint32_t x = cpu->r[Rm];
    cpu->r[Rd] = ((x & 0x00FF00FFu) << 8) | ((x & 0xFF00FF00u) >> 8);
}

// -----------------------------------------------------------------------------------

void handle_revsh(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;
    IGNORE_IF_PC(Rd);
    uint32_t x = cpu->r[Rm];
    uint32_t low = ((x & 0xFFu) << 8) | ((x >> 8) & 0xFFu);
    cpu->r[Rd] = (uint32_t)(int32_t)(int16_t)low;  // sign-extend
}

// -----------------------------------------------------------------------------------

void handle_rbit(exec_ctx_t *e)
{
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;

    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;

    uint32_t x = cpu->r[Rm];
    uint32_t y = 0;

    for (int i = 0; i < 32; i++) {
        y <<= 1;
        y |= (x & 1u);
        x >>= 1;
    }

    cpu->r[Rd] = y;
}