// src/cpu/alu.c  (context-based core + thin legacy wrappers)

#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "cpu_flags.h"
#include "alu.h"
#include "operand.h"    // dp_operand2(...), arm_read_src_reg(...)

// ===== helpers ===============================================================

// Write to PC via the execution context. If S=1 and Rd==PC, use exception-return.
static inline void write_pc_or_npc_ctx(exec_ctx_t *e, uint32_t value, bool s_bit)
{
    if (s_bit) {
        // Data-processing with S=1 and Rd==PC => exception return semantics
        // (CPSR := SPSR_<mode>, PC := value)
        cpu_exception_return(value);
    } else {
        // Plain branch via DP write to PC (ARM state keeps T unchanged)
        // Word align (bits[1:0] cleared).
        e->pc = value & ~3u;
    }
}

#ifndef BIT
#define BIT(x) (1u << (x))
#endif

// Only set N/Z; leave C/V unchanged (used by long-multiply ops)
static inline void set_nz_64_ctx(exec_ctx_t *e, uint64_t val) {
    CPU *c = &e->vm->cpu;
    if (val == 0) c->cpsr |=  BIT(30); else c->cpsr &= ~BIT(30);               // Z
    if (val & 0x8000000000000000ULL) c->cpsr |=  BIT(31); else c->cpsr &= ~BIT(31); // N
}

static inline bool mul_unpredictable_pc(uint32_t RdHi, uint32_t RdLo,
                                        uint32_t Rm, uint32_t Rs) {
    return (RdHi == 15u) | (RdLo == 15u) | (Rm == 15u) | (Rs == 15u);
}

// Small helpers to write flags cleanly
static inline void set_NZ(CPU *c, uint32_t res) {
    c->cpsr = (c->cpsr & ~((1u<<31)|(1u<<30))) | ((res & 0x80000000u)) | ((res==0)?(1u<<30):0);
}
static inline void set_C(CPU *c, uint32_t C) {
    c->cpsr = (c->cpsr & ~(1u<<29)) | ((C & 1u) << 29);
}
static inline void set_V(CPU *c, uint32_t V) {
    c->cpsr = (c->cpsr & ~(1u<<28)) | ((V & 1u) << 28);
}

// ===== Yup ============================================================

// Read a source register from context (r15 reads as (PC+8) & ~3)
static inline uint32_t read_src_reg_ctx(const exec_ctx_t *e, unsigned r) {
    if (r == 15u) {
        uint32_t pc = e->vm->cpu.r[15];
        return (pc + 8u) & ~3u;
    }
    return e->vm->cpu.r[r];
}

// Compatibility shim: let legacy call sites keep using arm_read_src_reg(r)
// as long as they have an `exec_ctx_t *e` in scope (which all handlers do).
#define arm_read_src_reg(r) read_src_reg_ctx(e, (r))

// ===== context-based core implementations ===================================

void handle_add(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    const int rn = (instr >> 16) & 0xF;
    const int rd = (instr >> 12) & 0xF;
    const bool S = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = (c->cpsr >> 29) & 1u;
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t op1 = arm_read_src_reg(rn);

    const uint64_t wide = (uint64_t)op1 + (uint64_t)op2;
    const uint32_t res  = (uint32_t)wide;

    if (rd == 15) { write_pc_or_npc_ctx(e, res, S); return; }
    c->r[rd] = res;

    if (S) {
        const uint32_t N = res >> 31;
        const uint32_t Z = (res == 0);
        const uint32_t C = (uint32_t)(wide >> 32);
        const uint32_t V = (~(op1 ^ op2) & (op1 ^ res)) >> 31;
        c->cpsr = (c->cpsr & ~((1u<<31)|(1u<<30)|(1u<<29)|(1u<<28)))
                | (N<<31) | (Z<<30) | (C<<29) | (V<<28);
    }
}

void handle_adc(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    const int rn = (instr >> 16) & 0xF;
    const int rd = (instr >> 12) & 0xF;
    const bool S = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = (c->cpsr >> 29) & 1u;
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t op1 = arm_read_src_reg(rn);
    const uint32_t cin = (c->cpsr >> 29) & 1u;

    const uint64_t wide = (uint64_t)op1 + (uint64_t)op2 + (uint64_t)cin;
    const uint32_t res  = (uint32_t)wide;

    if (rd == 15) { write_pc_or_npc_ctx(e, res, S); return; }
    c->r[rd] = res;

    if (S) {
        set_NZ(c, res);
        set_C(c, (uint32_t)((wide >> 32) & 1u));
        const uint32_t b = op2 + cin;
        const uint32_t V = (~(op1 ^ b) & (op1 ^ res)) >> 31;
        set_V(c, V);
    }
}

void handle_sub(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    const int rn = (instr >> 16) & 0xF;
    const int rd = (instr >> 12) & 0xF;
    const bool S = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = (c->cpsr >> 29) & 1u;
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t a   = arm_read_src_reg(rn);
    const uint32_t res = a - op2;

    if (rd == 15) { write_pc_or_npc_ctx(e, res, S); return; }
    c->r[rd] = res;

    if (S) {
        set_NZ(c, res);
        set_C(c, a >= op2); // NOT borrow
        const uint32_t V = ((a ^ op2) & (a ^ res) & 0x80000000u) != 0;
        set_V(c, V);
    }
}

void handle_sbc(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    const int rn = (instr >> 16) & 0xF;
    const int rd = (instr >> 12) & 0xF;
    const bool S = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = (c->cpsr >> 29) & 1u;
    const uint32_t op2    = dp_operand2(instr, &sh_carry);
    const uint32_t a      = arm_read_src_reg(rn);
    const uint32_t cin    = (c->cpsr >> 29) & 1u;
    const uint32_t borrow = 1u - (cin & 1u);

    const uint64_t wide = (uint64_t)a - (uint64_t)op2 - (uint64_t)borrow;
    const uint32_t res  = (uint32_t)wide;

    if (rd == 15) { write_pc_or_npc_ctx(e, res, S); return; }
    c->r[rd] = res;

    if (S) {
        set_NZ(c, res);
        set_C(c, ((wide >> 32) == 0)); // no borrow => C=1
        const uint32_t btot = op2 + borrow;
        const uint32_t V = ((a ^ btot) & (a ^ res) & 0x80000000u) != 0;
        set_V(c, V);
    }
}

void handle_rsb(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    const int rn = (instr >> 16) & 0xF;
    const int rd = (instr >> 12) & 0xF;
    const bool S = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = (c->cpsr >> 29) & 1u;
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t a   = arm_read_src_reg(rn);
    const uint32_t res = op2 - a;

    if (rd == 15) { write_pc_or_npc_ctx(e, res, S); return; }
    c->r[rd] = res;

    if (S) {
        set_NZ(c, res);
        set_C(c, op2 >= a);
        const uint32_t V = ((op2 ^ a) & (op2 ^ res) & 0x80000000u) != 0;
        set_V(c, V);
    }
}

void handle_rsc(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    const int rn = (instr >> 16) & 0xF;
    const int rd = (instr >> 12) & 0xF;
    const bool S = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = (c->cpsr >> 29) & 1u;
    const uint32_t op2    = dp_operand2(instr, &sh_carry);
    const uint32_t a      = arm_read_src_reg(rn);
    const uint32_t cin    = (c->cpsr >> 29) & 1u;
    const uint32_t borrow = 1u - (cin & 1u);

    const uint64_t wide = (uint64_t)op2 - (uint64_t)a - (uint64_t)borrow;
    const uint32_t res  = (uint32_t)wide;

    if (rd == 15) { write_pc_or_npc_ctx(e, res, S); return; }
    c->r[rd] = res;

    if (S) {
        set_NZ(c, res);
        set_C(c, ((wide >> 32) == 0));
        const uint32_t atot = a + borrow;
        const uint32_t V = ((op2 ^ atot) & (op2 ^ res) & 0x80000000u) != 0;
        set_V(c, V);
    }
}

void handle_and(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    const int rn = (instr >> 16) & 0xF;
    const int rd = (instr >> 12) & 0xF;
    const bool S = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = (c->cpsr >> 29) & 1u;
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t res = arm_read_src_reg(rn) & op2;

    if (rd == 15) { write_pc_or_npc_ctx(e, res, S); return; }
    c->r[rd] = res;

    if (S) {
        set_NZ(c, res);
        set_C(c, sh_carry & 1u);
    }
}

void handle_eor(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    const int rn = (instr >> 16) & 0xF;
    const int rd = (instr >> 12) & 0xF;
    const bool S = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = (c->cpsr >> 29) & 1u;
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t res = arm_read_src_reg(rn) ^ op2;

    if (rd == 15) { write_pc_or_npc_ctx(e, res, S); return; }
    c->r[rd] = res;

    if (S) {
        set_NZ(c, res);
        set_C(c, sh_carry & 1u);
    }
}

void handle_mov_dp(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    const bool S  = ((instr >> 20) & 1u) != 0;
    const int  Rd = (instr >> 12) & 0xF;

    uint32_t sh_carry = (c->cpsr >> 29) & 1u;
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t res = op2;

    if (Rd == 15) { write_pc_or_npc_ctx(e, res, S); return; }

    c->r[Rd] = res;
    if (S) {
        set_NZ(c, res);
        set_C(c, sh_carry & 1u); // MOVS: C := shifter carry
    }
}

void handle_mvn(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    const bool S  = ((instr >> 20) & 1u) != 0;
    const int  Rd = (instr >> 12) & 0xF;

    uint32_t sh_carry = (c->cpsr >> 29) & 1u;
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t res = ~op2;

    if (Rd == 15) { write_pc_or_npc_ctx(e, res, S); return; }
    c->r[Rd] = res;

    if (S) {
        set_NZ(c, res);
        set_C(c, sh_carry & 1u);
    }
}

void handle_cmp(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    const int rn = (instr >> 16) & 0xF;

    uint32_t sh_carry = (c->cpsr >> 29) & 1u;
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t a   = arm_read_src_reg(rn);
    const uint32_t res = a - op2;

    set_NZ(c, res);
    set_C(c, a >= op2);
    const uint32_t V = ((a ^ op2) & (a ^ res) & 0x80000000u) != 0;
    set_V(c, V);
}

void handle_tst(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    const int rn = (instr >> 16) & 0xF;

    uint32_t sh_carry = (c->cpsr >> 29) & 1u;
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t res = arm_read_src_reg(rn) & op2;

    set_NZ(c, res);
    set_C(c, sh_carry & 1u);
}

void handle_teq(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    const int rn = (instr >> 16) & 0xF;

    uint32_t sh_carry = (c->cpsr >> 29) & 1u;
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t res = arm_read_src_reg(rn) ^ op2;

    set_NZ(c, res);
    set_C(c, sh_carry & 1u);
}

void handle_cmn(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    const int rn = (instr >> 16) & 0xF;

    uint32_t sh_carry = (c->cpsr >> 29) & 1u;
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t a   = arm_read_src_reg(rn);
    const uint64_t sum = (uint64_t)a + (uint64_t)op2;
    const uint32_t res = (uint32_t)sum;

    set_NZ(c, res);
    set_C(c, (uint32_t)((sum >> 32) & 1u));
    const uint32_t V = (~(a ^ op2) & (a ^ res) & 0x80000000u) != 0;
    set_V(c, V);
}

// --- long multiply family ---

void handle_umull(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    uint32_t S    = (instr >> 20) & 1u;
    uint32_t RdHi = (instr >> 16) & 0xFu;
    uint32_t RdLo = (instr >> 12) & 0xFu;
    uint32_t Rs   = (instr >> 8)  & 0xFu;
    uint32_t Rm   =  instr        & 0xFu;

    if (mul_unpredictable_pc(RdHi, RdLo, Rm, Rs)) return;

    uint64_t res = (uint64_t)c->r[Rm] * (uint64_t)c->r[Rs];
    c->r[RdLo]  = (uint32_t)res;
    c->r[RdHi]  = (uint32_t)(res >> 32);
    if (S) set_nz_64_ctx(e, res);
}

void handle_umlal(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    uint32_t S    = (instr >> 20) & 1u;
    uint32_t RdHi = (instr >> 16) & 0xFu;
    uint32_t RdLo = (instr >> 12) & 0xFu;
    uint32_t Rs   = (instr >> 8)  & 0xFu;
    uint32_t Rm   =  instr        & 0xFu;

    if (mul_unpredictable_pc(RdHi, RdLo, Rm, Rs)) return;

    uint64_t acc = ((uint64_t)c->r[RdHi] << 32) | c->r[RdLo];
    uint64_t res = acc + (uint64_t)c->r[Rm] * (uint64_t)c->r[Rs];
    c->r[RdLo]  = (uint32_t)res;
    c->r[RdHi]  = (uint32_t)(res >> 32);
    if (S) set_nz_64_ctx(e, res);
}

void handle_smull(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    uint32_t S    = (instr >> 20) & 1u;
    uint32_t RdHi = (instr >> 16) & 0xFu;
    uint32_t RdLo = (instr >> 12) & 0xFu;
    uint32_t Rs   = (instr >> 8)  & 0xFu;
    uint32_t Rm   =  instr        & 0xFu;

    if (mul_unpredictable_pc(RdHi, RdLo, Rm, Rs)) return;

    int64_t res = (int64_t)(int32_t)c->r[Rm] * (int64_t)(int32_t)c->r[Rs];
    c->r[RdLo] = (uint32_t)res;
    c->r[RdHi] = (uint32_t)((uint64_t)res >> 32);
    if (S) set_nz_64_ctx(e, (uint64_t)res);
}

void handle_smlal(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    uint32_t S    = (instr >> 20) & 1u;
    uint32_t RdHi = (instr >> 16) & 0xFu;
    uint32_t RdLo = (instr >> 12) & 0xFu;
    uint32_t Rs   = (instr >> 8)  & 0xFu;
    uint32_t Rm   =  instr        & 0xFu;

    if (mul_unpredictable_pc(RdHi, RdLo, Rm, Rs)) return;

    int64_t acc = ((int64_t)(int32_t)c->r[RdHi] << 32) | c->r[RdLo];
    int64_t res = acc + (int64_t)(int32_t)c->r[Rm] * (int64_t)(int32_t)c->r[Rs];
    c->r[RdLo] = (uint32_t)res;
    c->r[RdHi] = (uint32_t)((uint64_t)res >> 32);
    if (S) set_nz_64_ctx(e, (uint64_t)res);
}

void handle_mul(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    uint32_t S  = (instr >> 20) & 1u;
    uint32_t Rd = (instr >> 16) & 0xFu;    // Rd
    uint32_t Rn = (instr >> 12) & 0xFu;    // must be 0 in MUL encoding
    uint32_t Rs = (instr >> 8)  & 0xFu;    // Rs
    uint32_t Rm =  instr        & 0xFu;    // Rm
    (void)Rn;

    uint32_t res = c->r[Rm] * c->r[Rs];
    c->r[Rd] = res;

    if (S) {
        // N/Z set, C/V UNPREDICTABLE (leave unchanged)
        c->cpsr = (c->cpsr & 0x3FFFFFFFu)
                | (res & 0x80000000u)
                | ((res == 0) ? 0x40000000u : 0);
    }
}

void handle_mla(exec_ctx_t *e) {
    CPU *c = &e->vm->cpu;
    const uint32_t instr = e->instr;
    uint32_t S  = (instr >> 20) & 1u;
    uint32_t Rd = (instr >> 16) & 0xFu;    // Rd
    uint32_t Rn = (instr >> 12) & 0xFu;    // accumulate
    uint32_t Rs = (instr >> 8)  & 0xFu;    // Rs
    uint32_t Rm =  instr        & 0xFu;    // Rm

    uint32_t res = c->r[Rm] * c->r[Rs] + c->r[Rn];
    c->r[Rd] = res;

    if (S) {
        c->cpsr = (c->cpsr & 0x3FFFFFFFu)
                | (res & 0x80000000u)
                | ((res == 0) ? 0x40000000u : 0);
    }
}
