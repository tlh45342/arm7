// src/cpu/execute.c  — K12 dispatcher stabilized, VM->CPU refactor
// - Uses exec_ctx_t for execution state passed to handlers
// - CPU is owned by VM: use e->vm->cpu everywhere (no e->cpu)
// - Stable typedefs: k12_fn_t, k12_entry_t
// - Condition gating via arm_condition_holds(exec_ctx_t*, cond)
// - xmask32 disambiguation preserved

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

#include "cpu.h"        // CPU, COND_*, arm_condition_holds
#include "log.h"
#include "disasm.h"
#include "vm.h"         // VM, vm_read_mem32 (if you fetch here)

// Handlers
#include "alu.h"
#include "branch.h"
#include "system.h"
#include "logic.h"
#include "memops.h"
#include "media.h"
#include "coproc.h"
#include "execute.h"

// ------------------------ stable types ------------------------
typedef void (*k12_fn_t)(exec_ctx_t *);

typedef struct {
    uint16_t    mask12;     // mask in 12-bit key space
    uint16_t    value12;    // value in 12-bit key space
    uint32_t    xmask32;    // OPTIONAL exact disambiguation (0 => none)
    uint32_t    xvalue32;   // value for xmask32
    bool        check_cond; // apply cond? (false for DEADBEEF/NOP/WFI/DSB/DMB/ISB)
    k12_fn_t    fn;         // handler
    const char *name;       // for logs
} k12_entry_t;

// ---------- forward ----------
static k12_fn_t k12_decode_ctx(exec_ctx_t *e, uint32_t instr);

// ------------------------ key12 helper ------------------------
static inline uint16_t key12(uint32_t instr) {
    uint32_t op1 = (instr >> 25) & 0x7;   // bits 27:25
    uint32_t op2 = (instr >> 20) & 0x1F;  // bits 24:20
    uint32_t op3 = (instr >>  4) & 0x0F;  // bits  7:4
    return (uint16_t)((op1 << 9) | (op2 << 4) | op3);
}

// -------------------- seed rules (order not critical; priority is) --------------------
static const k12_entry_t K12_TABLE[] = {
    // Short multiply group (S-agnostic; must outrank DP rows)
    { 0x0FFFu, 0x0019u, 0x0FE000F0u, 0x00000090u, true, handle_mul, "MUL/MULS" },
    { 0x0FFFu, 0x0019u, 0x0FF000F0u, 0x00200090u, true, handle_mla, "MLA/MLAS" },

    // Bitfield Clear (BFC), A32
    { 0x0FFFu, 0x07D1u, 0x0FE3C1FFu, 0x07C3001Fu, true, handle_bfc, "BFC" },
    { 0x0FFFu, 0x07C1u, 0x0FE3C1FFu, 0x07C3001Fu, true, handle_bfc, "BFC" },

    // Bitfield Insert (BFI), A32
    { 0x0FFFu, 0x07C1u, 0x0FE00070u, 0x07C00010u, true, handle_bfi, "BFI" },
    { 0x0FFFu, 0x07D1u, 0x0FE00070u, 0x07C00010u, true, handle_bfi, "BFI" },

    // BEFORE CMN
    { 0x0FFFu, 0x0161u, 0x0FFF0FF0u, 0x016F0F10u, true, handle_clz, "CLZ" },

	// RBIT: reverse bits in word
	// Example: E6FF1F30 -> key=0x6F3
	{ 0x0FFFu, 0x06F3u, 0x0FFF0FF0u, 0x06FF0F30u, true, handle_rbit, "RBIT" },

	// USAT: unsigned saturate; Match op1/op2 and fixed op3 bits, but allow shift bits to vary.
	{ 0x0FF3u, 0x06E1u, 0x0FE00030u, 0x06E00010u, true, handle_usat, "USAT" },

    // DSB/DMB/ISB
    { 0x0FFFu, 0x0574u, 0x0FFF0FF0u, 0x057F0040u, false, handle_dsb, "DSB" },
    { 0x0FFFu, 0x0575u, 0x0FFF0FF0u, 0x057F0050u, false, handle_dmb, "DMB" },
    { 0x0FFFu, 0x0576u, 0x0FFF0FF0u, 0x057F0060u, false, handle_isb, "ISB" },

    // LDR (literal)
    { 0x0F50u, 0x0510u, 0x033F0000u, 0x011F0000u, true, handle_ldr_literal, "LDR(literal)" },

    // BKPT
    { 0x0FFFu, 0x0127u, 0x0FF000F0u, 0x01200070u, false, handle_bkpt, "BKPT" },

    { 0x0F00u, 0x0100u, 0xFF00F000u, 0xF1000000u, true, handle_cps, "CPS" },

    // MSR
    { 0x0FFFu, 0x0120u, 0x0120F000u, 0x0120F000u, true, handle_msr, "MSR reg->CPSR" },
    { 0x0FFFu, 0x0320u, 0x0320F000u, 0x0320F000u, true, handle_msr, "MSR imm->CPSR" },

    // BLX (imm)
    { 0x0E00u, 0x0A00u, 0xFE000000u, 0xFA000000u, false, handle_blx_imm, "BLX (imm)" },

    // PUSH / POP
    { 0x0F10u, 0x0800u, 0x0FFF0000u, 0x092D0000u, true, handle_stm, "PUSH (STMDB sp!)" },
    { 0x0F10u, 0x0810u, 0x0FFF0000u, 0x08BD0000u, true, handle_ldm, "POP (LDMIA sp!)" },

    // LDM / STM generic
    { 0x0E10u, 0x0810u, 0, 0, true, handle_ldm, "LDM" },
    { 0x0E10u, 0x0800u, 0, 0, true, handle_stm, "STM" },

    // Halfword / signed load/store
    { 0x0E0Fu, 0x000Bu, 0x00500000u, 0x00400000u, true, handle_strh,  "STRH(imm)"  },
    { 0x0E0Fu, 0x000Bu, 0x00500000u, 0x00000000u, true, handle_strh,  "STRH(reg)"  },
    { 0x0E0Fu, 0x000Bu, 0x00500000u, 0x00500000u, true, handle_ldrh,  "LDRH(imm)"  },
    { 0x0E0Fu, 0x000Bu, 0x00500000u, 0x00100000u, true, handle_ldrh,  "LDRH(reg)"  },
    { 0x0E0Fu, 0x000Du, 0x00100000u, 0x00100000u, true, handle_ldrsb, "LDRSB"      },
    { 0x0E0Fu, 0x000Fu, 0x00100000u, 0x00100000u, true, handle_ldrsh, "LDRSH"      },

    // BX / BLX (reg)
    { 0x0FFFu, 0x0121u, 0x0FFFFFF0u, 0x012FFF10u, true, handle_bx,      "BX (reg)"  },
    { 0x0FFFu, 0x0123u, 0x0FFFFFF0u, 0x012FFF30u, true, handle_blx_reg, "BLX (reg)" },

    { 0x0DE0u, 0x01A0u, 0, 0, true, handle_mov, "MOV" },

    // MOVW / MOVT
    { 0x0F00u, 0x0300u, 0x0FF00000u, 0x03000000u, true, handle_movw, "MOVW" },
    { 0x0F00u, 0x0300u, 0x0FF00000u, 0x03400000u, true, handle_movt, "MOVT" },

    // Shift by register aliases
    { 0x0FE0u, 0x01A0u, 0x0FE000F0u, 0x01A00010u, true, handle_mov, "LSL (reg)" },
    { 0x0FE0u, 0x01A0u, 0x0FE000F0u, 0x01A00030u, true, handle_mov, "LSR (reg)" },
    { 0x0FE0u, 0x01A0u, 0x0FE000F0u, 0x01A00050u, true, handle_mov, "ASR (reg)" },
    { 0x0FE0u, 0x01A0u, 0x0FE000F0u, 0x01A00070u, true, handle_mov, "ROR (reg)" },

    // STR / STRB reg offset
    { 0x0E50u, 0x0600u, 0, 0, true, handle_str_regoffset,  "STR reg-offset" },
    { 0x0F7Fu, 0x0740u, 0, 0, true, handle_strb_reg_shift, "STRB reg LSL#0" },
    { 0x0F71u, 0x0740u, 0, 0, true, handle_strb_reg_shift, "STRB reg imm-shift" },

    // SWP / SWPB
    { 0x0E0Fu, 0x0009u, 0x0FB00FF0u, 0x01000090u, true, handle_swp,  "SWP"  },
    { 0x0E0Fu, 0x0009u, 0x0FB00FF0u, 0x01400090u, true, handle_swpb, "SWPB" },

    // MRS / MSR (SPSR variants)
    { 0x0FBFu, 0x0100u, 0x0FBF0FFFu, 0x014F0000u, true, handle_mrs, "MRS (SPSR)" },
    { 0x0FFFu, 0x0120u, 0x0FBF0FFFu, 0x0160F000u, true, handle_msr, "MSR reg->SPSR" },
    { 0x0FFFu, 0x0320u, 0x0FBF0F00u, 0x0360F000u, true, handle_msr, "MSR imm->SPSR" },

    // Doubleword transfers
    { 0x0E0Fu, 0x000Du, 0x00400000u, 0x00400000u, true, exec_ldrd_imm, "LDRD(imm)" },
    { 0x0E0Fu, 0x000Du, 0x00400000u, 0x00000000u, true, exec_ldrd_reg, "LDRD(reg)" },
    { 0x0E0Fu, 0x000Fu, 0x00400000u, 0x00400000u, true, exec_strd_imm, "STRD(imm)" },
    { 0x0E0Fu, 0x000Fu, 0x00400000u, 0x00000000u, true, exec_strd_reg, "STRD(reg)" },

    // ---- DP (register) class ----
    { 0x0DE0u, 0x0000u, 0, 0, true, handle_and, "AND" },
    { 0x0DE0u, 0x0020u, 0, 0, true, handle_eor, "EOR" },
    { 0x0DE0u, 0x0100u, 0, 0, true, handle_tst, "TST" },
    { 0x0DE0u, 0x0120u, 0, 0, true, handle_teq, "TEQ" },
    { 0x0DE0u, 0x0140u, 0, 0, true, handle_cmp, "CMP" },
    { 0x0DE0u, 0x0160u, 0, 0, true, handle_cmn, "CMN" },
    { 0x0DE0u, 0x0040u, 0, 0, true, handle_sub, "SUB" },
    { 0x0DE0u, 0x0060u, 0, 0, true, handle_rsb, "RSB" },
    { 0x0DE0u, 0x0080u, 0, 0, true, handle_add, "ADD" },
    { 0x0DE0u, 0x00A0u, 0, 0, true, handle_adc, "ADC" },
    { 0x0DE0u, 0x00C0u, 0, 0, true, handle_sbc, "SBC" },
    { 0x0DE0u, 0x00E0u, 0, 0, true, handle_rsc, "RSC" },
    { 0x0DE0u, 0x0180u, 0, 0, true, handle_orr, "ORR" },
    { 0x0DE0u, 0x01C0u, 0, 0, true, handle_bic, "BIC" },
    { 0x0DE0u, 0x01E0u, 0, 0, true, handle_mvn, "MVN" },

    // ---- DP (immediate) class ----
    { 0x0FE0u, 0x0200u, 0, 0, true, handle_and, "AND (imm)" },
    { 0x0FE0u, 0x0310u, 0, 0, true, handle_tst_imm, "TST (imm)" },
    { 0x0FE0u, 0x0330u, 0, 0, true, handle_teq, "TEQ (imm)" },
    { 0x0FE0u, 0x0350u, 0, 0, true, handle_cmp_imm, "CMP (imm)" },
    { 0x0FE0u, 0x0370u, 0, 0, true, handle_cmn, "CMN (imm)" },
    { 0x0FE0u, 0x0380u, 0, 0, true, handle_orr, "ORR (imm)" },
    { 0x0FE0u, 0x03A0u, 0, 0, true, handle_mov, "MOV (imm)" },
    { 0x0FE0u, 0x03C0u, 0, 0, true, handle_bic, "BIC (imm)" },
    { 0x0FE0u, 0x03E0u, 0, 0, true, handle_mvn, "MVN (imm)" },

    // ---- Single data transfer ----
    { 0x0F50u, 0x0510u, 0, 0, true, handle_ldr_preimm,   "LDR  pre-imm" },
    { 0x0F50u, 0x0500u, 0, 0, true, handle_str_preimm,   "STR  pre-imm" },
    { 0x0F50u, 0x0540u, 0, 0, true, handle_strb_preimm,  "STRB pre-imm" },
    { 0x0F50u, 0x0550u, 0, 0, true, handle_ldrb_preimm,  "LDRB pre-imm" },
    { 0x0F50u, 0x0440u, 0, 0, true, handle_strb_postimm, "STRB(post,imm)" },
    { 0x0F50u, 0x0410u, 0, 0, true, handle_ldr_postimm,  "LDR (post,imm)" },
    { 0x0F70u, 0x0400u, 0, 0, true, handle_str_postimm,  "STR (post,imm)" },
    { 0x0F70u, 0x0450u, 0, 0, true, handle_ldrb_postimm, "LDRB post-imm"  },
    { 0x0F70u, 0x0470u, 0, 0, true, handle_ldrb_postimm, "LDRB post-imm W"},
    { 0x0E50u, 0x0610u, 0, 0, true, handle_ldr_regoffset,"LDR reg-offset" },

    // Multiply-long
    { 0x0E0Fu, 0x0009u, 0x0FE000F0u, 0x00800090u, true, handle_umull, "UMULL" },
    { 0x0E0Fu, 0x0009u, 0x0FE000F0u, 0x00A00090u, true, handle_umlal, "UMLAL" },
    { 0x0E0Fu, 0x0009u, 0x0FE000F0u, 0x00C00090u, true, handle_smull, "SMULL" },
    { 0x0E0Fu, 0x0009u, 0x0FE000F0u, 0x00E00090u, true, handle_smlal, "SMLAL" },

    // LDRB (register) split
    { 0x0F7Fu, 0x0750u, 0, 0, true, handle_ldrb_reg,       "LDRB reg pre LSL#0" },
    { 0x0F71u, 0x0750u, 0, 0, true, handle_ldrb_reg_shift, "LDRB reg imm-shift" },

    // Stack-ish specials
    { 0x0FF0u, 0x0520u, 0x000F0000u, 0x000D0000u, true, handle_str_predec, "STR(pre-dec SP,imm)" },
    { 0x0FFFu, 0x0490u, 0x0FFFF000u, 0x049DF000u, true, handle_pop_pc,     "POP{..,pc}" },

    // Branch family
    { 0x0F00u, 0x0A00u, 0, 0, true, handle_b,  "B"  },
    { 0x0F00u, 0x0B00u, 0, 0, true, handle_bl, "BL" },

    // ---- Coprocessor 64-bit ARM-register transfers ----
    //
    // MCRR/MRRC use the conditional coprocessor transfer encoding.
    // MCRR2/MRRC2 use cond=1111 and therefore bypass normal condition gating.
    //
    // Observed MRRC2 example:
    //   FC510F0E -> key12 0xC50
    { 0x0FF0u, 0x0C40u, 0x0FF00000u, 0x0C400000u, true,  handle_mcrr,  "MCRR"  },
    { 0x0FF0u, 0x0C50u, 0x0FF00000u, 0x0C500000u, true,  handle_mrrc,  "MRRC"  },
    { 0x0FF0u, 0x0C40u, 0xFFF00000u, 0xFC400000u, false, handle_mcrr2, "MCRR2" },
    { 0x0FF0u, 0x0C50u, 0xFFF00000u, 0xFC500000u, false, handle_mrrc2, "MRRC2" },

    // System / exact 32-bit patterns
    { 0x0F00u, 0x0F00u, 0x0F000000u, 0x0F000000u, true,  handle_svc, "SVC" },
    { 0x0FBFu, 0x0100u, 0x0FBF0FFFu, 0x010F0000u, true,  handle_mrs, "MRS" },
    { 0x0FFFu, 0x0320u, 0xFFFFFFFFu, 0xE320F000u, false, handle_nop, "NOP" },
    { 0x0FFFu, 0x0320u, 0xFFFFFFFFu, 0xE320F003u, false, handle_wfi, "WFI" },

    // Easter egg / halt
    { 0x0FFFu, 0x0EAEu, 0xFFFFFFFFu, 0xDEADBEEFu, false, handle_deadbeef, "DEADBEEF" },
};

// -------------------- 4096 per-key candidate lists --------------------
#define KEY12_SPACE 4096
#define MAX_PER_KEY 16

static uint16_t g_keylist[KEY12_SPACE][MAX_PER_KEY];
static uint8_t  g_keyprio[KEY12_SPACE][MAX_PER_KEY];
static uint8_t  g_keycount[KEY12_SPACE];
static bool     g_k12_ready = false;

static inline uint8_t popcnt16(uint16_t x){
    x = (x & 0x5555u) + ((x >> 1) & 0x5555u);
    x = (x & 0x3333u) + ((x >> 2) & 0x3333u);
    x = (x + (x >> 4)) & 0x0F0Fu;
    x = x + (x >> 8);
    return (uint8_t)(x & 0x1Fu);
}
static inline uint8_t popcnt32(uint32_t x){
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0F0F0F0Fu;
    x = x + (x >> 8);
    x = x + (x >> 16);
    return (uint8_t)(x & 0x3Fu);
}

static inline uint8_t k12_priority(const k12_entry_t *e) {
    uint8_t p = popcnt16(e->mask12);
    p += popcnt32(e->xmask32);
    return p;
}

static void k12_build_table(void) {
    for (int k = 0; k < KEY12_SPACE; ++k) g_keycount[k] = 0;

    const size_t N = sizeof(K12_TABLE)/sizeof(K12_TABLE[0]);
    for (int k = 0; k < KEY12_SPACE; ++k) {
        uint8_t cnt = 0;
        for (uint16_t ei = 0; ei < N; ++ei) {
            const k12_entry_t *e = &K12_TABLE[ei];
            if (((uint16_t)k & e->mask12) == e->value12) {
                uint8_t pr = k12_priority(e);

                if (cnt < MAX_PER_KEY) {
                    uint8_t pos = cnt;
                    while (pos > 0 && pr > g_keyprio[k][pos-1]) {
                        g_keyprio[k][pos] = g_keyprio[k][pos-1];
                        g_keylist[k][pos] = g_keylist[k][pos-1];
                        --pos;
                    }
                    g_keyprio[k][pos] = pr;
                    g_keylist[k][pos] = ei;
                    ++cnt;
                } else {
                    if (pr > g_keyprio[k][cnt-1]) {
                        uint8_t pos = cnt - 1;
                        while (pos > 0 && pr > g_keyprio[k][pos-1]) {
                            g_keyprio[k][pos] = g_keyprio[k][pos-1];
                            g_keylist[k][pos] = g_keylist[k][pos-1];
                            --pos;
                        }
                        g_keyprio[k][pos] = pr;
                        g_keylist[k][pos] = ei;
                    }
                }
            }
        }
        g_keycount[k] = cnt;
    }
    g_k12_ready = true;
}

static inline void k12_ensure_built(void) {
    if (!g_k12_ready) k12_build_table();
}

// File-private, context-aware decoder.
// Resolves an instruction to a handler using the K12 keyed table.
// No globals (e.g., cpu_current) used; everything flows through exec_ctx_t *e.
static k12_fn_t k12_decode_ctx(exec_ctx_t *e, uint32_t instr)
{
    // Build decode tables on first use
    k12_ensure_built();

    const uint16_t k   = key12(instr);
    const uint8_t  cnt = g_keycount[k];

    for (uint8_t i = 0; i < cnt; ++i) {
        const k12_entry_t *ent = &K12_TABLE[g_keylist[k][i]];

        // Extra mask/value check for entries that need it
        if (ent->xmask32 && ((instr & ent->xmask32) != ent->xvalue32))
            continue;

        // Optional ARM condition check (if the entry requires it)
        if (ent->check_cond) {
            const uint8_t cond = (instr >> 28) & 0xF;
            if (cond != COND_AL) {
                // Seed a temp context from the caller; ensure instr is set
                exec_ctx_t ex = *e;
                ex.instr = instr;

                if (!arm_condition_holds(&ex, cond)) {
                    // Architecturally a NOP when condition fails
                    return handle_nop;
                }
            }
        }

        if (debug_flags & DBG_K12)
            log_printf("[K12] %s match (key=0x%03X)\n", ent->name, k);

        return ent->fn;
    }

    if (debug_flags & DBG_K12)
        log_printf("[K12] no match (key=0x%03X)\n", k);

    return NULL; // caller should handle unknown-instruction behavior
}

// --------------------------- cpu_execute ---------------------------
bool cpu_execute(exec_ctx_t *e)
{
    if (!e || !e->vm) return false;

    CPU *cpu = &e->vm->cpu;

    // Default next PC = sequential. Handlers (branch/BX/etc.) will overwrite npc.
    cpu->npc = cpu->r[15] + 4;

    // Use your existing fetch path or keep the instr already set by the CLI/stepper.
    // If you fetch here:
    // if (!vm_read_mem32(e->vm, e->pc, &e->instr)) { return false; }

    // After you've fetched into e->instr and before you decode/execute:
    if (debug_flags & DBG_DISASM) {
        char dline[128];
        disasm_line(e->pc, e->instr, dline, sizeof dline);
        log_printf("%08X:       %08X        %s\n", e->pc, e->instr, dline);
    }
    if (debug_flags & DBG_TRACE) {
        log_printf("[TRACE] PC=0x%08X Instr=0x%08X\n", e->pc, e->instr);
    }
    if (debug_flags & DBG_K12) {
        uint32_t op1 = (e->instr >> 25) & 0x7;
        uint32_t op2 = (e->instr >> 20) & 0x1F;
        uint32_t op3 = (e->instr >>  4) & 0xF;
        uint32_t key = (op1 << 9) | (op2 << 4) | op3;
        log_printf("[K12] key=0x%03X op1=%u op2=%u op3=%u\n",
                   key, op1, op2, op3);
    }

    // Decode
    k12_fn_t fn = k12_decode_ctx(e, e->instr);

    if (!fn) {
        if (debug_flags & DBG_INSTR) {
            log_printf("[EXEC] unknown instr 0x%08X at PC=0x%08X; treating as NOP+advance\n",
                       e->instr, e->pc);
        }
        // Commit default advance and return
        cpu->r[15] = cpu->npc;
        e->pc      = cpu->npc;
        return true;
    }

    // Execute handler
    fn(e);

    // Count this instruction
    cpu->cycles++;

    // If the handler requested exit, stop here (no commit advance)
    if (cpu->halted || cpu->halt_reason != HALT_NONE) {
        return false;
    }

    // Commit (idempotent if handler already set npc)
    cpu->r[15] = cpu->npc;
    e->pc      = cpu->npc;
    return true;
}
