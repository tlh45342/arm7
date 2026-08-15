// src/cpu/cpu.c  — global-free shim layer
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#include "cpu.h"
#include "mem.h"
#include "hw.h"
#include "execute.h"
#include "debug.h"
#include "log.h"
#include "vm.h"

extern bool trace_all;
extern debug_flags_t debug_flags;

// -----------------------------------------------------------------------------
// Run-state control
// -----------------------------------------------------------------------------
// We no longer keep a process-wide "halted" bit. Prefer per-CPU or VM flags.
// These shims are kept only if other code still calls them. They do NOT use globals.

void cpu_halt(void)       {
    // Prefer a CPU-local flag; if your CPU struct uses another name, adjust here.
    // This is a no-op if nobody calls it anymore (VM should drive exits).
    // Intentionally empty to avoid reintroducing globals.
}

void cpu_clear_halt(void) {
    // See comment in cpu_halt()
}

bool cpu_is_halted(void)  {
    // VM should query CPU/VM state directly; returning false keeps legacy sites harmless.
    return false;
}

// -----------------------------------------------------------------------------
// Legacy utilities (removed):
// - arm_read_src_reg(int r): replaced by ctx-aware helper in operand.c
// - cpu_fetch(): VM+execute.c own fetch/advance now
// - execute_one_instruction() / cpu_step(): VM drives stepping via cpu_execute(&e)
// - cpu_dump_registers(): move to a CLI routine that prints from an explicit CPU*
// - cpu_get_spsr_current() / cpu_exception_return(): wire through explicit context
// -----------------------------------------------------------------------------

// Keep a small, explicit register dumper that takes a CPU* so callers can switch.
void cpu_dump_registers_of(const CPU *c) {
    if (!c) { log_printf("Registers: <null cpu>\n"); return; }
    log_printf("Registers:\n");
    for (int i = 0; i < 16; i++) {
        log_printf("r%-2d = 0x%08x  ", i, c->r[i]);
        if ((i + 1) % 4 == 0) log_printf("\n");
    }
}

// If some code still calls the old no-arg dumper, keep a harmless stub.
// (We’ll migrate callers to pass an explicit CPU* soon.)
void cpu_dump_registers(void) {
    log_printf("cpu_dump_registers(): legacy no-arg call; please migrate to cpu_dump_registers_of(cpu)\n");
}

// -----------------------------------------------------------------------------
// Exception/exit helpers
// -----------------------------------------------------------------------------

// If/when you add banked SPSRs for modes, route this through the bank.
// Legacy function had no way to know "current" CPU without a global.
// Keep a minimal form that the new pipeline doesn’t need.
static inline uint32_t cpu_get_spsr_current(void) {
    return 0; // not used; retained only to satisfy legacy callers if any remain
}

void cpu_exception_return(uint32_t new_pc) {
    // No global "current CPU" available here anymore; the context/handler must set npc.
    // Keep as a stub so legacy code links; migrate callers to write e->cpu->npc directly.
    (void)new_pc;
}

// Map new CPUX_* reasons to your existing halt_reason_t without globals.
// Callers pass an explicit CPU*; no process-wide state needed.
void cpu_request_exit(CPU *c, cpu_exit_t why, uint32_t code, uint32_t pc) {
    (void)code; (void)pc;
    if (!c) return;

    switch (why) {
        case CPUX_BKPT:  c->halt_reason = HALT_BKPT;  break;
        case CPUX_UNDEF: c->halt_reason = HALT_UNDEF; break;
        case CPUX_HLT:   c->halt_reason = HALT_SWI;   break;  // or a dedicated HALT if you have one
        case CPUX_PANIC: /* fallthrough */
        case CPUX_NONE:  /* fallthrough */
        default:         c->halt_reason = HALT_ABORT; break;
    }

    c->halted = true;
}
