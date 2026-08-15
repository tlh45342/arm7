#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"     // CPU type, CPSR_* masks, exec_ctx_t
#include "arm-vm.h"
#include "log.h"
#include "vm.h"

// --------------------- LEGACY (kept for compatibility) ---------------------
// NOTE: These rely on the global 'cpu' instance and are used by older handlers.
extern CPU cpu;

static inline uint32_t cpsr_get_C(void) { return (cpu.cpsr >> 29) & 1u; }
static inline uint32_t cpsr_get_V(void) { return (cpu.cpsr >> 28) & 1u; }

static inline void cpsr_set_NZ(uint32_t result) {
    if (result & 0x80000000u) cpu.cpsr |=  BIT(31); else cpu.cpsr &= ~BIT(31);
    if (result == 0)          cpu.cpsr |=  BIT(30); else cpu.cpsr &= ~BIT(30);
}

static inline void cpsr_set_C_from(uint32_t c) {
    if (c) cpu.cpsr |= BIT(29);
    else   cpu.cpsr &= ~BIT(29);
}

static inline uint32_t cpsr_mode(void) { return cpu.cpsr & CPSR_MODE_MASK; }
static inline int      is_user_mode(void) { return (cpsr_mode() == 0x10u); }

// Inline CPSR flag helpers (legacy/global)
static inline void cpu_set_flag_N(bool v) { if (v) cpu.cpsr |=  CPSR_N; else cpu.cpsr &= ~CPSR_N; }
static inline void cpu_set_flag_Z(bool v) { if (v) cpu.cpsr |=  CPSR_Z; else cpu.cpsr &= ~CPSR_Z; }
static inline void cpu_set_flag_C(bool v) { if (v) cpu.cpsr |=  CPSR_C; else cpu.cpsr &= ~CPSR_C; }
static inline void cpu_set_flag_V(bool v) { if (v) cpu.cpsr |=  CPSR_V; else cpu.cpsr &= ~CPSR_V; }

void psr_write(uint32_t *psr, uint32_t value, uint32_t fields, int is_cpsr);

static inline uint32_t cpsr_get_Q(void)   { return (cpu.cpsr >> 27) & 1u; }
static inline void     cpsr_set_Q(void)   { cpu.cpsr |= CPSR_Q; }        // sticky set
static inline void     cpsr_clear_Q(void) { cpu.cpsr &= ~CPSR_Q; }       // for tests/tools


// --------------------- NEW CONTEXT-BASED HELPERS ---------------------------
// These are the versions your refactored handlers should call.
// No global 'cpu' usage; everything goes through e->vm->cpu.

static inline void cpsr_set_NZ_ctx(exec_ctx_t *e, uint32_t result)
{
    uint32_t cpsr = e->vm->cpu.cpsr;

    if (result & 0x80000000u) cpsr |= CPSR_N; else cpsr &= ~CPSR_N;
    if (result == 0)          cpsr |= CPSR_Z; else cpsr &= ~CPSR_Z;

    e->vm->cpu.cpsr = cpsr;
}

static inline void cpsr_set_C_from_ctx(exec_ctx_t *e, uint32_t c)
{
    if (c)
        e->vm->cpu.cpsr |= CPSR_C;
    else
        e->vm->cpu.cpsr &= ~CPSR_C;
}

static inline void cpsr_set_V_ctx(exec_ctx_t *e, bool v)
{
    if (v)
        e->vm->cpu.cpsr |= CPSR_V;
    else
        e->vm->cpu.cpsr &= ~CPSR_V;
}

static inline void cpu_set_flag_N_ctx(exec_ctx_t *e, bool v)
{
    if (v)
        e->vm->cpu.cpsr |= CPSR_N;
    else
        e->vm->cpu.cpsr &= ~CPSR_N;
}

static inline void cpu_set_flag_Z_ctx(exec_ctx_t *e, bool v)
{
    if (v)
        e->vm->cpu.cpsr |= CPSR_Z;
    else
        e->vm->cpu.cpsr &= ~CPSR_Z;
}

static inline void cpu_set_flag_C_ctx(exec_ctx_t *e, bool v)
{
    if (v)
        e->vm->cpu.cpsr |= CPSR_C;
    else
        e->vm->cpu.cpsr &= ~CPSR_C;
}

static inline void cpu_set_flag_V_ctx(exec_ctx_t *e, bool v)
{
    if (v)
        e->vm->cpu.cpsr |= CPSR_V;
    else
        e->vm->cpu.cpsr &= ~CPSR_V;
}

static inline uint32_t cpsr_get_C_ctx(exec_ctx_t *e)
{
    return (e->vm->cpu.cpsr >> 29) & 1u;
}

static inline uint32_t cpsr_get_V_ctx(exec_ctx_t *e)
{
    return (e->vm->cpu.cpsr >> 28) & 1u;
}

static inline uint32_t cpsr_get_Q_ctx(exec_ctx_t *e)
{
    return (e->vm->cpu.cpsr >> 27) & 1u;
}

static inline void cpsr_set_Q_ctx(exec_ctx_t *e)
{
    e->vm->cpu.cpsr |= CPSR_Q;    // sticky set
}

static inline void cpsr_clear_Q_ctx(exec_ctx_t *e)
{
    e->vm->cpu.cpsr &= ~CPSR_Q;
}
