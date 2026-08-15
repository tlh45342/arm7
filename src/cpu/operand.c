// operand.c — context-only Operand2 decode (no globals)
#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "vm.h"       // VM owns CPU: e->vm->cpu
#include "operand.h"

// ---- CPSR bits (A-profile) ----
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

static inline uint32_t cpsr_get_C(const exec_ctx_t *e) {
    return (e->vm->cpu.cpsr & CPSR_C) ? 1u : 0u;
}

// ARM rule: when a GP register is read as a *source*, r15 reads as (PC+8) aligned.
static inline uint32_t ctx_reg_read_src_local(const exec_ctx_t *e, unsigned r) {
    if (r == 15u) {
        uint32_t pc = e->vm->cpu.r[15];
        return (pc + 8u) & ~3u;
    }
    return e->vm->cpu.r[r];
}

// Rotate right helper (amount mod 32), amount must be in [0..31]
static inline uint32_t ror32(uint32_t x, unsigned s) {
    s &= 31u;
    return (x >> s) | (x << (32u - s));
}

// Shift-by-immediate semantics (for Operand2 when bit[4]==0)
static uint32_t op2_shift_imm(exec_ctx_t *e,
                              uint32_t rm_val,
                              unsigned type,
                              unsigned amount,
                              uint32_t *sh_carry)
{
    (void)e;  // ctx not needed here; keep signature consistent

    // *sh_carry is both input (incoming C) and output (shifter carry out)
    uint32_t inC = *sh_carry & 1u;

    switch (type) {
        case 0: // LSL
            if (amount == 0) { /* carry unchanged */ return rm_val; }
            if (amount < 32) {
                *sh_carry = (rm_val >> (32 - amount)) & 1u;
                return rm_val << amount;
            } else if (amount == 32) {
                *sh_carry = rm_val & 1u;
                return 0u;
            } else {
                *sh_carry = 0u;
                return 0u;
            }

        case 1: // LSR
            if (amount == 0) { // interpreted as 32
                *sh_carry = (rm_val >> 31) & 1u;
                return 0u;
            }
            if (amount < 32) {
                *sh_carry = (rm_val >> (amount - 1)) & 1u;
                return rm_val >> amount;
            } else {
                *sh_carry = 0u;
                return 0u;
            }

        case 2: // ASR
            if (amount == 0) { // interpreted as 32
                *sh_carry = (rm_val >> 31) & 1u;
                return (rm_val & 0x80000000u) ? 0xFFFFFFFFu : 0u;
            }
            if (amount < 32) {
                *sh_carry = (rm_val >> (amount - 1)) & 1u;
                // arithmetic right shift: replicate sign bit
                if (rm_val & 0x80000000u) {
                    return (rm_val >> amount) | ~(0xFFFFFFFFu >> amount);
                } else {
                    return rm_val >> amount;
                }
            } else {
                *sh_carry = (rm_val >> 31) & 1u;
                return (rm_val & 0x80000000u) ? 0xFFFFFFFFu : 0u;
            }

        case 3: // ROR / RRX
            if (amount == 0) {
                // RRX: rotate right through carry by 1
                *sh_carry = rm_val & 1u;
                return (inC << 31) | (rm_val >> 1);
            } else {
                unsigned s = amount & 31u;
                if (s == 0) { // rotate by 32 is identity; carry becomes bit31
                    *sh_carry = (rm_val >> 31) & 1u;
                    return rm_val;
                }
                *sh_carry = (rm_val >> (s - 1)) & 1u;
                return ror32(rm_val, s);
            }

        default:
            // unreachable
            return rm_val;
    }
}

// Shift-by-register semantics (Operand2 when bit[4]==1)
static uint32_t op2_shift_reg(exec_ctx_t *e,
                              uint32_t rm_val,
                              unsigned type,
                              uint32_t rs_val,
                              uint32_t *sh_carry)
{
    (void)e;  // ctx not needed here; keep signature consistent

    // Only the bottom 8 bits of Rs are used for the shift amount
    unsigned amount = rs_val & 0xFFu;
    if (amount == 0) {
        // No shift, carry unchanged
        return rm_val;
    }

    switch (type) {
        case 0: // LSL
            if (amount < 32) {
                *sh_carry = (rm_val >> (32 - amount)) & 1u;
                return rm_val << amount;
            } else if (amount == 32) {
                *sh_carry = rm_val & 1u;
                return 0u;
            } else {
                *sh_carry = 0u;
                return 0u;
            }

        case 1: // LSR
            if (amount < 32) {
                *sh_carry = (rm_val >> (amount - 1)) & 1u;
                return rm_val >> amount;
            } else if (amount == 32) {
                *sh_carry = (rm_val >> 31) & 1u;
                return 0u;
            } else {
                *sh_carry = 0u;
                return 0u;
            }

        case 2: // ASR
            if (amount < 32) {
                *sh_carry = (rm_val >> (amount - 1)) & 1u;
                if (rm_val & 0x80000000u) {
                    return (rm_val >> amount) | ~(0xFFFFFFFFu >> amount);
                } else {
                    return rm_val >> amount;
                }
            } else {
                *sh_carry = (rm_val >> 31) & 1u;
                return (rm_val & 0x80000000u) ? 0xFFFFFFFFu : 0u;
            }

        case 3: // ROR
        {
            unsigned s = amount & 31u;
            if (s == 0) {
                // rotate by 32*n: result is rm_val; carry becomes bit31
                *sh_carry = (rm_val >> 31) & 1u;
                return rm_val;
            }
            *sh_carry = (rm_val >> (s - 1)) & 1u;
            return ror32(rm_val, s);
        }

        default:
            return rm_val;
    }
}

uint32_t dp_operand2_ctx(exec_ctx_t *e, uint32_t instr, uint32_t *sh_carry)
{
    // Seed carry from CPSR.C if caller didn't provide a pointer
    uint32_t local_c = cpsr_get_C(e);
    if (!sh_carry) {
        sh_carry = &local_c;
    } else {
        *sh_carry &= 1u; // sanitize to 0/1
    }

    // Decode Operand2
    uint32_t I = (instr >> 25) & 1u;
    if (I) {
        // Immediate form: imm8 rotated right by 2*rot
        uint32_t imm8 = instr & 0xFFu;
        unsigned rot  = ((instr >> 8) & 0xFu) * 2u;
        if (rot == 0) {
            // shifter carry unchanged
            return imm8;
        } else {
            uint32_t val = ror32(imm8, rot & 31u);
            *sh_carry = (val >> 31) & 1u;
            return val;
        }
    } else {
        // Register/shift form
        unsigned rm      = instr & 0xFu;
        uint32_t rm_val  = ctx_reg_read_src_local(e, rm);
        unsigned type    = (instr >> 5) & 0x3u;
        unsigned reg_sh  = (instr >> 4) & 1u;

        if (!reg_sh) {
            unsigned amount = (instr >> 7) & 0x1Fu; // imm5
            return op2_shift_imm(e, rm_val, type, amount, sh_carry);
        } else {
            unsigned rs      = (instr >> 8) & 0xFu;
            uint32_t rs_val  = ctx_reg_read_src_local(e, rs);
            return op2_shift_reg(e, rm_val, type, rs_val, sh_carry);
        }
    }
}
