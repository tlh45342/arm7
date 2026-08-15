// include/cpu.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "debug.h"

// ===== Exit/Halt reasons =====
typedef enum {
	CPUX_NONE=0,
    CPUX_BKPT,
	CPUX_FAULT,
	CPUX_HLT,
	CPUX_UNDEF,
	CPUX_PANIC
} cpu_exit_t;

typedef enum {
    HALT_NONE     = 0,
    HALT_DEADBEEF = 1,
    HALT_BKPT     = 2,
    HALT_SWI      = 3,
    HALT_UNDEF    = 4,
    HALT_ABORT    = 5
} halt_reason_t;

struct VM;

// ===== Canonical CPU type =====
// (Use this everywhere; delete any cpu_t/struct cpu usage from headers)
typedef struct CPU {
    uint64_t      cycles;
    uint32_t      r[16];        // R0..R15 (R15 = PC)
    uint32_t      cpsr;         // Current PSR
    uint32_t      spsr;         // shadow PSR used by SVC/MSR/MR
    uint32_t      npc;          // next PC (fall-through or branch target)
    bool          halted;       // run/stop latch
    halt_reason_t halt_reason;  // VM stop cause
    bool          exit_request;
    cpu_exit_t    exit_reason;  // higher-level exit (bkpt, undef, panic…)
    uint32_t      exit_pc;
    uint32_t      exit_code;
} CPU;

// Forward declaration only; full definition lives in cpu.h
typedef struct CPU CPU;

// Forward declare VM so we can hold a pointer in exec_ctx_t
struct VM;

// Minimal execution context used by cpu_execute and handlers
typedef struct exec_ctx {
    struct VM *vm;     // <--- REQUIRED: cpu_execute fetch uses this
    uint32_t  pc;
    uint32_t  npc;     // preferred next PC (A+4 for ARM)
    uint32_t  instr;
} exec_ctx_t;

// Ensure this is declared here too
void cpu_request_exit(CPU *c, cpu_exit_t why, uint32_t code, uint32_t pc);

// Initializer
static inline void make_ctx(exec_ctx_t *e, struct VM *v, uint32_t pc, uint32_t ins) {
    if (!e || !v) return;
    e->vm    = v;
    e->pc    = pc;
    e->npc   = pc + 4u;
    e->instr = ins;
}

// Commit pc -> r15 (defined in cpu_exec.c to avoid requiring complete CPU here)
void ctx_commit_pc(exec_ctx_t *e);

void cpu_step_ctx(exec_ctx_t *e);

// ===== Public configuration =====
#ifndef ENTRY_POINT
#define ENTRY_POINT 0x8000u
#endif

#ifndef BIT
#define BIT(x) (1u << (x))
#endif

// CPSR bit masks
#define CPSR_N   BIT(31)
#define CPSR_Z   BIT(30)
#define CPSR_C   BIT(29)
#define CPSR_V   BIT(28)
#define CPSR_Q   BIT(27)
#define CPSR_GE0 BIT(16)
#define CPSR_GE1 BIT(17)
#define CPSR_GE2 BIT(18)
#define CPSR_GE3 BIT(19)
#define CPSR_GE_MASK (BIT(16)|BIT(17)|BIT(18)|BIT(19))
#define CPSR_E   BIT(9)
#define CPSR_A   BIT(8)
#define CPSR_I   BIT(7)
#define CPSR_F   BIT(6)
#define CPSR_T   BIT(5)
#define CPSR_MODE_MASK 0x1Fu

// ===== x ======
// Condition codes
#define COND_EQ 0u
#define COND_NE 1u
#define COND_CS 2u
#define COND_CC 3u
#define COND_MI 4u
#define COND_PL 5u
#define COND_VS 6u
#define COND_VC 7u
#define COND_HI 8u
#define COND_LS 9u
#define COND_GE 10u
#define COND_LT 11u
#define COND_GT 12u
#define COND_LE 13u
#define COND_AL 14u
#define COND_NV 15u

bool arm_condition_holds(const exec_ctx_t *e, uint32_t cond);

// ===== Global run-state (temporary; move to VM later) =====
extern uint64_t cycle;        // global cycle counter
extern bool     cpu_halted;   // compatibility latch
extern debug_flags_t debug_flags;
extern bool          trace_all;

// ===== CPU API =====
uint32_t arm_read_src_reg(int r);

// Execute *one* instruction (legacy dispatcher)
bool     execute(uint32_t instr);

// Centralized fetch (legacy; mutates cpu.npc)
uint32_t cpu_fetch(void);

// Backtrace API (owned by cpu.c)
#ifndef CPU_MAX_BACKTRACE
#define CPU_MAX_BACKTRACE 64
#endif
void     cpu_bt_push(uint32_t lr);
void     cpu_bt_pop(void);
int      cpu_bt_depth(void);
uint32_t cpu_bt_frame(int i);
void     cpu_dump_backtrace(void);
void     cpu_dump_registers(void);
void     dump_registers(void);  // compat wrapper

// Halt state
void     cpu_halt(void);
void     cpu_clear_halt(void);
bool     cpu_is_halted(void);

void     cpu_step(void);
bool     cpu_execute(exec_ctx_t *e);

// Exceptions/exit
void     cpu_exception_return(uint32_t new_pc);
void     cpu_request_exit(CPU *c, cpu_exit_t why, uint32_t code, uint32_t pc);
