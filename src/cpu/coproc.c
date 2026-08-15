// src/cpu/coproc.c
#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "vm.h"
#include "debug.h"
#include "log.h"
#include "coproc.h"

extern debug_flags_t debug_flags;

static arm7_coproc_xfer64_t decode_xfer64(exec_ctx_t *e, bool is_cp2)
{
    uint32_t instr = e->instr;
    arm7_coproc_xfer64_t x;

    x.rt     = (uint8_t)((instr >> 12) & 0xFu);
    x.rt2    = (uint8_t)((instr >> 16) & 0xFu);
    x.cp     = (uint8_t)((instr >>  8) & 0xFu);
    x.opc1   = (uint8_t)((instr >>  4) & 0xFu);
    x.crm    = (uint8_t)( instr        & 0xFu);
    x.is_cp2 = is_cp2;
    return x;
}

bool arm7_coproc_read64(exec_ctx_t *e,
                        const arm7_coproc_xfer64_t *xfer,
                        uint64_t *value)
{
    (void)e;
    (void)xfer;
    if (value) *value = 0;
    return false;
}

bool arm7_coproc_write64(exec_ctx_t *e,
                         const arm7_coproc_xfer64_t *xfer,
                         uint64_t value)
{
    (void)e;
    (void)xfer;
    (void)value;
    return false;
}

static void handle_mrrc_common(exec_ctx_t *e, bool is_cp2)
{
    CPU *cpu = &e->vm->cpu;
    arm7_coproc_xfer64_t x = decode_xfer64(e, is_cp2);
    uint64_t value = 0;
    bool implemented = arm7_coproc_read64(e, &x, &value);

    if (implemented) {
        cpu->r[x.rt]  = (uint32_t)(value & 0xFFFFFFFFu);
        cpu->r[x.rt2] = (uint32_t)(value >> 32);

        if (debug_flags & DBG_INSTR) {
            log_printf("[%s] p%u opc1=%u c%u -> r%u:r%u = 0x%08X:%08X\n",
                       is_cp2 ? "MRRC2" : "MRRC",
                       x.cp, x.opc1, x.crm, x.rt2, x.rt,
                       cpu->r[x.rt2], cpu->r[x.rt]);
        }
    } else {
        if (debug_flags & DBG_INSTR) {
            log_printf("[%s] p%u opc1=%u c%u recognized; backend not implemented; r%u/r%u unchanged\n",
                       is_cp2 ? "MRRC2" : "MRRC",
                       x.cp, x.opc1, x.crm, x.rt, x.rt2);
        }
    }
}

static void handle_mcrr_common(exec_ctx_t *e, bool is_cp2)
{
    CPU *cpu = &e->vm->cpu;
    arm7_coproc_xfer64_t x = decode_xfer64(e, is_cp2);

    uint64_t value =
        ((uint64_t)cpu->r[x.rt2] << 32) |
        (uint64_t)cpu->r[x.rt];

    bool implemented = arm7_coproc_write64(e, &x, value);

    if (debug_flags & DBG_INSTR) {
        if (implemented) {
            log_printf("[%s] r%u:r%u -> p%u opc1=%u c%u\n",
                       is_cp2 ? "MCRR2" : "MCRR",
                       x.rt2, x.rt, x.cp, x.opc1, x.crm);
        } else {
            log_printf("[%s] p%u opc1=%u c%u recognized; backend not implemented; write ignored\n",
                       is_cp2 ? "MCRR2" : "MCRR",
                       x.cp, x.opc1, x.crm);
        }
    }
}

void handle_mrrc (exec_ctx_t *e) { handle_mrrc_common(e, false); }
void handle_mrrc2(exec_ctx_t *e) { handle_mrrc_common(e, true);  }
void handle_mcrr (exec_ctx_t *e) { handle_mcrr_common(e, false); }
void handle_mcrr2(exec_ctx_t *e) { handle_mcrr_common(e, true);  }
