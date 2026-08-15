// ============================
// File: src/cpu/media.c (context-based, VM-owned CPU)
// ============================
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#include "cpu.h"         // CPU definition (for e->vm->cpu.r[])
#include "cpu_flags.h"   // cpsr_set_NZ_ctx(exec_ctx_t*, ...), cpsr_set_Q_ctx(exec_ctx_t*)

/* Quick accessors: VM owns CPU now */
#define REG(n)     (e->vm->cpu.r[(n)])
#define SET_NZ(v)  cpsr_set_NZ_ctx(e, (v))
#define SET_Q()    cpsr_set_Q_ctx(e)

/* ---------- tiny helpers ---------- */
static inline uint32_t ror32(uint32_t x, unsigned n) {
    n &= 31u;
    return (x >> n) | (x << (32u - n));
}
static inline uint32_t asr32_u(uint32_t x, unsigned n) {
    if (n == 0) return x;
    if (n >= 32) return (x & 0x80000000u) ? 0xFFFFFFFFu : 0u;
    uint32_t signmask = (x & 0x80000000u) ? (~0u << (32u - n)) : 0u;
    return (x >> n) | signmask;
}
static inline int32_t sign_extend_width(uint32_t v, unsigned width) {
    if (width == 0 || width >= 32) return (int32_t)v;
    uint32_t m = 1u << (width - 1);
    uint32_t mask = (1u << width) - 1u;
    v &= mask;
    return (int32_t)((v ^ m) - m);
}
static inline uint32_t zero_extend_width(uint32_t v, unsigned width) {
    if (width >= 32) return v;
    return v & ((1u << width) - 1u);
}
static inline uint32_t pre_rotate(uint32_t rm, unsigned rot8) {
    return ror32(rm, rot8 & 31u);
}

/* ---------- SBFX / UBFX ---------- */
void handle_sbfx(exec_ctx_t *e, uint32_t instr) {
    uint32_t Rd    = (instr >> 12) & 0xF;
    uint32_t Rn    = (instr >> 16) & 0xF;
    uint32_t lsb   = (instr >> 7)  & 0x1F;
    uint32_t width = ((instr >> 16) & 0x1F) + 1u;  // width-1 in [20:16]
    uint32_t slice = REG(Rn) >> lsb;
    uint32_t res   = (uint32_t)sign_extend_width(slice, width);
    REG(Rd) = res;
    SET_NZ(res);
}

void handle_ubfx(exec_ctx_t *e, uint32_t instr) {
    uint32_t Rd    = (instr >> 12) & 0xF;
    uint32_t Rn    = (instr >> 16) & 0xF;
    uint32_t lsb   = (instr >> 7)  & 0x1F;
    uint32_t width = ((instr >> 16) & 0x1F) + 1u;
    uint32_t res   = zero_extend_width(REG(Rn) >> lsb, width);
    REG(Rd) = res;
    SET_NZ(res);
}

/* ---------- SSAT / USAT ---------- */
static inline int32_t ssat_apply(int32_t val, unsigned sat_bits, bool *did_sat) {
    int32_t maxv = (sat_bits >= 31) ? INT32_MAX : ((1 << (sat_bits - 1)) - 1);
    int32_t minv = (sat_bits >= 31) ? INT32_MIN : (-(1 << (sat_bits - 1)));
    if (val > maxv) { if (did_sat) *did_sat = true; return maxv; }
    if (val < minv) { if (did_sat) *did_sat = true; return minv; }
    return val;
}
static inline uint32_t usat_apply(int32_t val, unsigned sat_bits, bool *did_sat) {
    uint64_t maxv = (sat_bits >= 32) ? 0xFFFFFFFFull : ((1ull << sat_bits) - 1ull);
    if (val < 0)              { if (did_sat) *did_sat = true; return 0; }
    if ((uint64_t)val > maxv) { if (did_sat) *did_sat = true; return (uint32_t)maxv; }
    return (uint32_t)val;
}

void handle_ssat(exec_ctx_t *e, uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xF, Rn = (instr >> 0) & 0xF;
    unsigned sat = ((instr >> 16) & 0x1F) + 1u;  // 1..32
    unsigned sh  = (instr >> 7)  & 0x1F;
    bool asr     = ((instr >> 6) & 1u) != 0;
    int32_t x = (int32_t)REG(Rn);
    int32_t shifted = asr ? (int32_t)asr32_u((uint32_t)x, sh) : (int32_t)(x << sh);
    bool q=false; int32_t out = ssat_apply(shifted, sat, &q);
    REG(Rd) = (uint32_t)out;
    if (q) SET_Q();
}

void handle_usat(exec_ctx_t *e) {
    uint32_t instr = e->instr;

    uint32_t Rd = (instr >> 12) & 0xF;
    uint32_t Rn = instr & 0xF;

    /*
     * For USAT, bits[20:16] are the unsigned saturation width directly.
     * USAT #8 means clamp to 0..255.
     */
    unsigned sat = (instr >> 16) & 0x1F;

    unsigned sh = (instr >> 7) & 0x1F;
    bool asr = ((instr >> 6) & 1u) != 0;

    int32_t x = (int32_t)REG(Rn);
    int32_t shifted = asr
        ? (int32_t)asr32_u((uint32_t)x, sh)
        : (int32_t)(x << sh);

    bool q = false;
    uint32_t out = usat_apply(shifted, sat, &q);

    REG(Rd) = out;

    if (q) {
        SET_Q();
    }
}

/* ---------- SSAT16 / USAT16 ---------- */
static inline uint16_t lane_ssat16(int16_t v, unsigned sat, bool *q) {
    bool s=false; int32_t out = ssat_apply((int32_t)v, sat, &s);
    if (s && q) *q = true;
    return (uint16_t)out;
}
static inline uint16_t lane_usat16(uint16_t v, unsigned sat, bool *q) {
    bool s=false; uint32_t out = usat_apply((int32_t)v, sat, &s);
    if (s && q) *q = true;
    return (uint16_t)out;
}

void handle_ssat16(exec_ctx_t *e, uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xF, Rn = (instr >> 0) & 0xF;
    unsigned sat = ((instr >> 16) & 0xF) + 1u;  // 1..16
    unsigned sh  = (instr >> 7)  & 0x3;        // small LSL 0..3
    uint32_t x = REG(Rn) << sh;
    bool q=false;
    uint16_t lo = (uint16_t)(x & 0xFFFF);
    uint16_t hi = (uint16_t)((x >> 16) & 0xFFFF);
    uint16_t out_lo = lane_ssat16((int16_t)lo, sat, &q);
    uint16_t out_hi = lane_ssat16((int16_t)hi, sat, &q);
    REG(Rd) = ((uint32_t)out_hi << 16) | out_lo;
    if (q) SET_Q();
}

void handle_usat16(exec_ctx_t *e, uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xF, Rn = (instr >> 0) & 0xF;
    unsigned sat = ((instr >> 16) & 0xF) + 1u;  // 1..16
    unsigned sh  = (instr >> 7)  & 0x3;        // small LSL 0..3
    uint32_t x = REG(Rn) << sh;
    bool q=false;
    uint16_t lo = (uint16_t)(x & 0xFFFF);
    uint16_t hi = (uint16_t)((x >> 16) & 0xFFFF);
    uint16_t out_lo = lane_usat16(lo, sat, &q);
    uint16_t out_hi = lane_usat16(hi, sat, &q);
    REG(Rd) = ((uint32_t)out_hi << 16) | out_lo;
    if (q) SET_Q();
}

/* ---------- SXTB / SXTB16 / SXTH / UXTB / UXTB16 / UXTH ---------- */
void handle_sxtb(exec_ctx_t *e, uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xF, Rm = (instr >> 0) & 0xF;
    unsigned rot = ((instr >> 10) & 3u) * 8u; // 0,8,16,24
    uint32_t t = pre_rotate(REG(Rm), rot);
    REG(Rd) = (uint32_t)(int32_t)(int8_t)(t & 0xFF);
}

void handle_uxtb(exec_ctx_t *e, uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xF, Rm = (instr >> 0) & 0xF;
    unsigned rot = ((instr >> 10) & 3u) * 8u;
    uint32_t t = pre_rotate(REG(Rm), rot);
    REG(Rd) = t & 0xFFu;
}

void handle_sxth(exec_ctx_t *e, uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xF, Rm = (instr >> 0) & 0xF;
    unsigned rot = ((instr >> 10) & 1u) * 8u; // 0 or 8
    uint32_t t = pre_rotate(REG(Rm), rot);
    REG(Rd) = (uint32_t)(int32_t)(int16_t)(t & 0xFFFF);
}

void handle_uxth(exec_ctx_t *e, uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xF, Rm = (instr >> 0) & 0xF;
    unsigned rot = ((instr >> 10) & 1u) * 8u; // 0 or 8
    uint32_t t = pre_rotate(REG(Rm), rot);
    REG(Rd) = t & 0xFFFFu;
}

void handle_sxtb16(exec_ctx_t *e, uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xF, Rm = (instr >> 0) & 0xF;
    unsigned rot = ((instr >> 10) & 1u) * 8u; // 0 or 8
    uint32_t t = pre_rotate(REG(Rm), rot);
    int16_t lo = (int8_t)(t & 0xFF);
    int16_t hi = (int8_t)((t >> 16) & 0xFF);
    REG(Rd) = ((uint32_t)(uint16_t)hi << 16) | (uint16_t)lo;
}

void handle_uxtb16(exec_ctx_t *e, uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xF, Rm = (instr >> 0) & 0xF;
    unsigned rot = ((instr >> 10) & 1u) * 8u; // 0 or 8
    uint32_t t = pre_rotate(REG(Rm), rot);
    uint16_t lo = (uint8_t)(t & 0xFF);
    uint16_t hi = (uint8_t)((t >> 16) & 0xFF);
    REG(Rd) = ((uint32_t)hi << 16) | lo;
}

/* ---------- group shims for execute.c ---------- */
void handle_group_bitfield_saturate(exec_ctx_t *e, uint32_t I) {
    // If bit[23]==1 → Saturation family; else → Bitfield (BFX)
    if ((I >> 23) & 1u) {
        uint32_t op = (I >> 20) & 0x7;         // coarse sub-op
        if ((op & 0x6) == 0x2)      { handle_ssat(e, I);   return; }  // SSAT
        else if ((op & 0x6) == 0x6) { handle_usat(e);   return; }  // USAT
        bool is_unsigned = ((I >> 22) & 1u) != 0;
        if (((I >> 5) & 1u) == 1u) {                         // *SAT16
            if (is_unsigned) handle_usat16(e, I);
            else             handle_ssat16(e, I);
            return;
        }
        if (is_unsigned) handle_usat(e);
        else             handle_ssat(e, I);   // fallback
    } else {
        bool is_unsigned = ((I >> 21) & 1u) != 0; // U bit in BFX space
        if (is_unsigned) handle_ubfx(e, I);
        else             handle_sbfx(e, I);
    }
}

void handle_group_extend(exec_ctx_t *e, uint32_t I) {
    uint32_t op = (I >> 20) & 0x7; // coarse sub-op
    switch (op) {
        case 0b010: // sign extends
            if ((I >> 5) & 1u)      { handle_sxtb16(e, I); return; }
            if ((I >> 6) & 1u)      { handle_sxth(e, I);   return; }
            /* else */                handle_sxtb(e, I);   return;
        case 0b011: // zero extends
            if ((I >> 5) & 1u)      { handle_uxtb16(e, I); return; }
            if ((I >> 6) & 1u)      { handle_uxth(e, I);   return; }
            /* else */                handle_uxtb(e, I);   return;
        default:
            return; // let outer decoder flag unknown if misrouted
    }
}
