#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "vm.h"   // needed so we can access e->vm->cpu

/* Forward declare to avoid heavy includes */
typedef struct exec_ctx exec_ctx_t;

/* ------------------- Register access helpers (no globals) ------------------- */

/* Read a register as a *source operand* (ARM state: PC yields (PC&~3)+8) */
static inline uint32_t ctx_reg_read_src(exec_ctx_t *e, int r) {
    if (r == 15)
        return ((e->vm->cpu.r[15] & ~3u) + 8u);
    return e->vm->cpu.r[r];
}

/* Read a register raw (no PC+8 semantics) */
static inline uint32_t ctx_reg_read_raw(exec_ctx_t *e, int r) {
    return e->vm->cpu.r[r];
}

/* Write a general register raw */
static inline void ctx_reg_write(exec_ctx_t *e, int r, uint32_t val) {
    e->vm->cpu.r[r] = val;
}

/* Write PC via next-PC (keeps ARM word alignment), for control-flow updates */
static inline void ctx_write_pc_via_npc(exec_ctx_t *e, uint32_t new_pc) {
    e->vm->cpu.npc = new_pc & ~3u;
}

/* Convenience accessor */
static inline uint32_t ctx_pc_src(exec_ctx_t *e) {
    return ctx_reg_read_src(e, 15);
}

/* ---------------- Addressing mode helpers used by LDR/STR families --------- */

/* Shift by immediate used in AddrMode2/“extra” encodings */
static inline uint32_t ctx_am2_shift_imm(uint32_t val, uint32_t stype, uint32_t sh_imm) {
    switch (stype & 3u) {
        case 0: /* LSL */
            return (sh_imm == 0) ? val : (val << sh_imm);
        case 1: /* LSR */
            return (sh_imm == 0) ? 0u : (val >> sh_imm);
        case 2: /* ASR */
            if (sh_imm == 0) sh_imm = 32;
            return (uint32_t)((int32_t)val >> (sh_imm & 31));
        case 3: /* ROR / RRX */
        default: {
            if (sh_imm == 0) {
                /* RRX with carry-in = 0 (minimal form) */
                return (val >> 1);
            }
            uint32_t n = sh_imm & 31u;
            return (val >> n) | (val << (32 - n));
        }
    }
}

/* Rm shifted by immediate (AddrMode2 register offset path) */
static inline uint32_t ctx_am2_reg_scaled(exec_ctx_t *e, uint32_t instr) {
    uint32_t Rm    =  instr        & 0xFu;
    uint32_t stype = (instr >> 5)  & 0x3u;
    uint32_t shimm = (instr >> 7)  & 0x1Fu;
    return ctx_am2_shift_imm(ctx_reg_read_src(e, (int)Rm), stype, shimm);
}

/* Extra load/store (halfword/signed) effective address */
static inline uint32_t ctx_extra_addr(exec_ctx_t *e,
                                      uint32_t instr,
                                      uint32_t base,
                                      uint32_t *new_base_out,
                                      bool     *do_wb_out)
{
    uint32_t P = (instr >> 24) & 1u;
    uint32_t U = (instr >> 23) & 1u;
    uint32_t I = (instr >> 22) & 1u;
    uint32_t W = (instr >> 21) & 1u;

    uint32_t imm8 = ((instr >> 4) & 0xF0u) | (instr & 0x0Fu);
    uint32_t off  = I ? imm8 : ctx_reg_read_src(e, (int)(instr & 0xFu));

    int32_t  delta   = U ? (int32_t)off : -(int32_t)off;
    uint32_t ea      = P ? (uint32_t)((int32_t)base + delta) : base;
    uint32_t newbase = (uint32_t)((int32_t)base + delta);
    bool     do_wb   = (P && W) || (!P);

    if (new_base_out) *new_base_out = newbase;
    if (do_wb_out)    *do_wb_out    = do_wb;
    return ea;
}

/* Shared AddrMode2 computation for word/byte LDR/STR with pre/post & writeback */
static inline void ctx_am2_addr_common(exec_ctx_t *e,
                                       uint32_t instr,
                                       uint32_t base,
                                       uint32_t *ea_out,
                                       uint32_t *new_base_out,
                                       bool     *do_wb_out)
{
    uint32_t P = (instr >> 24) & 1u;
    uint32_t U = (instr >> 23) & 1u;
    uint32_t W = (instr >> 21) & 1u;
    uint32_t I = (instr >> 25) & 1u;

    uint32_t off;
    if (!I) {
        off = instr & 0xFFFu; /* imm12 */
    } else {
        if ((instr & (1u << 4)) != 0) {
            off = 0; /* unhandled form -> neutral offset */
        } else {
            uint32_t Rm    = instr & 0xFu;
            uint32_t stype = (instr >> 5) & 0x3u;
            uint32_t shimm = (instr >> 7) & 0x1Fu;
            off = ctx_am2_shift_imm(ctx_reg_read_src(e, (int)Rm), stype, shimm);
        }
    }

    uint32_t delta    = U ? off : (uint32_t)(-(int32_t)off);
    uint32_t new_base = base + delta;
    uint32_t ea       = P ? new_base : base;
    bool do_wb        = (P && W) || (!P);

    if (ea_out)       *ea_out       = ea;
    if (new_base_out) *new_base_out = new_base;
    if (do_wb_out)    *do_wb_out    = do_wb;
}
