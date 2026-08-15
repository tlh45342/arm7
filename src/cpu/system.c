// src/cpu/system.c — context-based (exec_ctx_t*) version
// - No global 'cpu' or 'cpu_current' references
// - All state reads/writes go through e->vm->cpu
// - psr_write() is used with pointers to cpu->{cpsr,spsr}

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>   // for PRIu64

#include "cpu.h"        // CPU struct, cpu_request_exit(), cpu_halt_reason enums, etc.
#include "cpu_flags.h"  // psr_write(), CPSR_* bit masks
#include "system.h"     // prototypes
#include "debug.h"
#include "vm.h"         // VM, so we can use e->vm->cpu

extern debug_flags_t debug_flags;

void psr_write(uint32_t *psr, uint32_t value, uint32_t fields, int is_cpsr)
{
    uint32_t p = *psr;
    const uint32_t F = 1u<<3, S = 1u<<2, X = 1u<<1, C = 1u<<0;

    if (fields & C) {
        uint32_t keep = p & ~(CPSR_E|CPSR_A|CPSR_I|CPSR_F|CPSR_T|CPSR_MODE_MASK);
        uint32_t set  = value & (CPSR_E|CPSR_A|CPSR_I|CPSR_F|CPSR_MODE_MASK);
        p = keep | set;
    }
    if (fields & X) { /* ignored */ }
    if (fields & S) { p = (p & ~CPSR_GE_MASK) | (value & CPSR_GE_MASK); }
    if (fields & F) {
        uint32_t keep = p & ~(CPSR_N|CPSR_Z|CPSR_C|CPSR_V|CPSR_Q);
        uint32_t set  = value &  (CPSR_N|CPSR_Z|CPSR_C|CPSR_V|CPSR_Q);
        p = keep | set;
    }

    *psr = p;

    if (debug_flags & DBG_INSTR) {
        log_printf("[PSR write] %s <= 0x%08X (fields: %c%c%c%c) -> 0x%08X\n",
            is_cpsr ? "CPSR" : "SPSR", value,
            (fields&F)?'f':'-', (fields&S)?'s':'-', (fields&X)?'x':'-', (fields&C)?'c':'-',
            *psr);
    }
}

// Small local helper to avoid legacy is_user_mode() that reads globals
static inline bool is_user_mode_ctx(exec_ctx_t *e) {
    return ((e->vm->cpu.cpsr & 0x1Fu) == 0x10u); // 0b10000 = User mode
}

// --- Barriers: treat as NOPs in this VM ---
void handle_dsb(exec_ctx_t *e) { (void)e; }
void handle_dmb(exec_ctx_t *e) { (void)e; }
void handle_isb(exec_ctx_t *e) { (void)e; }

// --- Software interrupt / SVC ---
// Architectural return address for SVC is the next instruction in the original
// context. With an NPC model, that’s exactly cpu->npc.
void handle_svc(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;
    (void)e; // imm not used in this VM (still keep e for signature)

    // Save old CPSR into SPSR_<svc> and LR_<svc> := return address
    cpu->spsr  = cpu->cpsr;
    cpu->r[14] = cpu->npc;                 // preferred return address

    // Enter SVC: mode=0b10011, T=0 (ARM), I=1, clear IT bits
    uint32_t p = cpu->cpsr;
    p = (p & ~0x1Fu) | 0x13u;              // SVC mode
    p &= ~(1u << 5);                       // T=0 (ARM)
    p |=  (1u << 7);                       // I=1 (mask IRQ)
    p &= ~((0x3Fu << 10) | (0x3u << 25));  // clear IT bits
    cpu->cpsr = p;

    // Vector to SVC handler @ 0x00000008 (A profile, low vectors)
    cpu->npc = 0x00000008u;
}

// --- MRS (move PSR to register) ---
// Rd==PC is UNPREDICTABLE; we ignore the write in that case.
void handle_mrs(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint32_t Rd  = (instr >> 12) & 0xFu;
    uint32_t sps = (instr >> 22) & 1u;   // 0=CPSR, 1=SPSR
    uint32_t val = sps ? cpu->spsr : cpu->cpsr;

    if (Rd == 15u) return;               // ignore (keeps core robust)
    cpu->r[Rd] = val;
}

// --- MSR helpers ---
static void handle_msr_common_ctx(exec_ctx_t *e, uint32_t instr) {
    CPU *cpu       = &e->vm->cpu;
    int spsr_sel    = (instr >> 22) & 1u;      // 0=CPSR, 1=SPSR
    uint32_t fields = (instr >> 16) & 0xFu;    // {f s x c}
    uint32_t op;

    if ((instr >> 25) & 1u) {
        // immediate form
        uint32_t imm8 = instr & 0xFFu;
        uint32_t rot2 = ((instr >> 8) & 0xFu) * 2u;
        op = rot2 ? ((imm8 >> rot2) | (imm8 << (32 - rot2))) : imm8;
    } else {
        // register form
        uint32_t Rm = instr & 0xFu;
        op = cpu->r[Rm];
    }

	if (debug_flags & DBG_INSTR) {
			log_printf("[MSR dbg] instr=0x%08X spsr_sel=%u fields=0x%X op=0x%08X\n",
					   instr, spsr_sel, fields, op);
		}

    if (spsr_sel) {
        if (!is_user_mode_ctx(e)) {
            psr_write(&cpu->spsr, op, fields, /*is_cpsr=*/0);
        }
    } else {
        // psr_write masks fields per privilege and handles control bits (E, AIF, T, mode)
        psr_write(&cpu->cpsr, op, fields, /*is_cpsr=*/1);
        // If T bit changed (interworking), fetch/commit glue will observe it on next step.
    }
}

void handle_msr     (exec_ctx_t *e) { handle_msr_common_ctx(e, e->instr); }
void handle_msr_reg (exec_ctx_t *e) { handle_msr_common_ctx(e, e->instr); }
void handle_msr_imm (exec_ctx_t *e) { handle_msr_common_ctx(e, e->instr); }

// --- SETEND (endian select) ---
void handle_setend(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint32_t E = (instr >> 9) & 1u;
    if (E) cpu->cpsr |=  CPSR_E;
    else   cpu->cpsr &= ~CPSR_E;
}

// --- CPS (change processor state / mask bits / mode change) ---
void handle_cps(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    bool disable = ((instr >> 18) & 1u) != 0;
    bool Mbit    = ((instr >> 17) & 1u) != 0;

    uint32_t mask = 0;
    if (instr & (1u << 8)) mask |= CPSR_A;
    if (instr & (1u << 7)) mask |= CPSR_I;
    if (instr & (1u << 6)) mask |= CPSR_F;

    if (disable) cpu->cpsr |= mask;
    else         cpu->cpsr &= ~mask;

    if (Mbit) {
        uint32_t mode = instr & 0x1Fu;
        cpu->cpsr = (cpu->cpsr & ~0x1Fu) | (mode & 0x1Fu);
        // If you later bank SP/LR/SPSR per-mode, adjust here.
    }
}

// -----------------------------------------------------------------------------
// Misc simple handlers referenced by execute()
// -----------------------------------------------------------------------------
void handle_nop(exec_ctx_t *e) { (void)e; }
void handle_wfi(exec_ctx_t *e) { (void)e; }   // NOP in this VM

void handle_deadbeef(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

#ifdef CPUX_DEADBEEF
    cpu_request_exit(cpu,
                     CPUX_DEADBEEF,
                     0,
                     cpu->r[15] ? (cpu->r[15] - 4) : 0);
#else
    cpu_request_exit(cpu,
                     CPUX_BKPT,
                     0xDEAD,
                     cpu->r[15] ? (cpu->r[15] - 4) : 0);
#endif
}

// BKPT
void handle_bkpt(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;
    uint32_t instr = e->instr;

    // imm16 = imm4: [19:16], imm12: [11:0]
    uint16_t imm4  = (instr >> 16) & 0x000Fu;
    uint16_t imm12 =  instr        & 0x0FFFu;
    uint16_t imm16 = (imm4 << 12) | imm12;

    // stop ON the BKPT address
    cpu->npc = e->pc;

    log_printf("[BKPT]");

    // request exit; run loop/execute() must honor this
    cpu_request_exit(cpu, CPUX_BKPT, imm16, e->pc);
}

// BFC Rd, #lsb, #width   (A32)
// Rd==PC is UNPREDICTABLE: ignore the write.
void handle_bfc(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint32_t Rd  = (instr >> 12) & 0xFu;
    uint32_t lsb = (instr >> 7)  & 0x1Fu;
    uint32_t msb = (instr >> 16) & 0x1Fu;

    if (msb < lsb) return;               // UNPREDICTABLE → ignore
    if (Rd == 15u) return;               // UNPREDICTABLE → ignore

    uint32_t raw_width = msb - lsb + 1u;
    uint32_t width     = (raw_width >= 32u) ? 32u : raw_width;

    uint32_t clear_mask = (width == 32u) ? 0u
                          : ~(((1u << width) - 1u) << lsb);

    cpu->r[Rd] &= clear_mask;
    // Flags unaffected
}

// BFI Rd, Rn, #lsb, #width  (A32)
// Rd==PC is UNPREDICTABLE: ignore the write.
void handle_bfi(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint32_t Rd  = (instr >> 12) & 0xFu;
    uint32_t Rn  = (instr >> 0)  & 0xFu;
    uint32_t lsb = (instr >> 7)  & 0x1Fu;
    uint32_t msb = (instr >> 16) & 0x1Fu;

    if (msb < lsb) return;               // UNPREDICTABLE → ignore
    if (Rd == 15u) return;               // UNPREDICTABLE → ignore

    uint32_t raw_width = msb - lsb + 1u;
    uint32_t width     = (raw_width >= 32u) ? 32u : raw_width;

    uint32_t field_mask = (width == 32u) ? 0xFFFFFFFFu
                          : ((1u << width) - 1u) << lsb;

    uint32_t src = cpu->r[Rn];
    uint32_t ins = (width == 32u) ? src : ((src << lsb) & field_mask);

    cpu->r[Rd] = (cpu->r[Rd] & ~field_mask) | ins;
    // Flags unaffected.
}

// CLZ Rd, Rm
// Rd==PC is UNPREDICTABLE: ignore the write.
void handle_clz(exec_ctx_t *e) {
    CPU *cpu = &e->vm->cpu;

    uint32_t instr = e->instr;
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;
    if (Rd == 15u) return;               // ignore

    uint32_t x  = cpu->r[Rm];
    uint32_t n;
#if defined(__GNUC__) || defined(__clang__)
    n = (x == 0) ? 32u : (uint32_t)__builtin_clz(x);
#else
    if (x == 0) {
        n = 32u;
    } else {
        n = 0u;
        while ((x & 0x80000000u) == 0u) { x <<= 1; n++; }
    }
#endif
    cpu->r[Rd] = n;
    // Flags unaffected.
}
