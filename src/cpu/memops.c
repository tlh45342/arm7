// memops.c (NPC-aware, VM-owned CPU)
// - Address reads use ctx_reg_read_src(e, ...) so Rn/Rm==15 read as PC+8 (ARM).
// - Any load that writes PC uses ctx_write_pc_via_npc(e, ...) instead of touching r15 directly.

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "cpu.h"
#include "vm.h"       // VM owns CPU: e->vm->cpu
#include "mem.h"      // mem_read8/16/32, mem_write8/16/32 (VM-based)
#include "log.h"      // log_printf
#include "memops.h"   // prototypes
#include "debug.h"
#include "cond.h"     // for evaluate_condition()
#include "operand.h"
#include "ctx_helpers.h"

// Convenience macro for register writes
#define CPU   (e->vm->cpu)
#define REG(n) (CPU.r[(n)])

// --------------------------- Implementations ---------------------------

void handle_str_postimm(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t rd    = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t ea, new_base; bool wb;
    ctx_am2_addr_common(e, instr, base, &ea, &new_base, &wb);

    mem_write32(e->vm, ea, REG(rd));
    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[STR] [0x%08X] <= r%u (0x%08X) %s\n",
                   ea, rd, REG(rd), wb ? "(wb)" : "");
}

void handle_ldrb_reg(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t rd    = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t ea = base + ctx_am2_reg_scaled(e, instr);

    uint8_t val = mem_read8(e->vm, ea);
    REG(rd) = val;

    if (debug_flags & DBG_INSTR)
        log_printf("[LDRB reg] r%u <= [0x%08X] => 0x%02X\n",
                   rd, ea, val);
}

void handle_strb_preimm(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn = (instr >> 16) & 0xFu;
    uint32_t rd = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t ea, new_base; bool wb;
    ctx_am2_addr_common(e, instr, base, &ea, &new_base, &wb);

    mem_write8(e->vm, ea, (uint8_t)REG(rd));
    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[STRB pre-imm] [0x%08X] <= r%u (0x%08X) %s\n",
                   ea, rd, REG(rd), wb ? "(wb)" : "");
}

void handle_stm(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t w     = (instr >> 21) & 1u;
    uint32_t U     = (instr >> 23) & 1u;
    uint32_t P     = (instr >> 24) & 1u;
    uint32_t regs  = instr & 0xFFFFu;

    uint32_t base = REG(rn);
    unsigned count = 0;
    for (unsigned i = 0; i < 16; ++i)
        if (regs & (1u << i)) ++count;

    /*
     * ARM multiple-transfer rule: registers are always mapped to ascending
     * memory addresses in ascending register-number order. P/U select the
     * address range; they do NOT reverse register order.
     *
     * IA: first=base
     * IB: first=base+4
     * DA: first=base-4*(n-1)
     * DB: first=base-4*n
     */
    uint32_t addr;
    if (U)
        addr = base + (P ? 4u : 0u);
    else
        addr = base - 4u * count + (P ? 0u : 4u);

    uint32_t first = addr;
    for (unsigned i = 0; i < 16; ++i) {
        if (regs & (1u << i)) {
            mem_write32(e->vm, addr, REG(i));
            addr += 4u;
        }
    }

    if (w)
        REG(rn) = U ? (base + 4u * count) : (base - 4u * count);

    if (debug_flags & DBG_INSTR)
        log_printf("[STM] r%u base=0x%08X regs=0x%04X first=0x%08X => wb=0x%08X\n",
                   rn, base, regs, first, REG(rn));
}

void handle_strb_postimm(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn = (instr >> 16) & 0xFu;
    uint32_t rd = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t ea, new_base; bool wb;
    ctx_am2_addr_common(e, instr, base, &ea, &new_base, &wb);

    mem_write8(e->vm, ea, (uint8_t)REG(rd));
    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[STRB post-imm] [0x%08X] <= r%u (0x%08X) %s\n",
                   ea, rd, REG(rd), wb ? "(wb)" : "");
}

void handle_str_preimm(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t rd    = (instr >> 12) & 0xFu;

    //log_printf("ENTER handle_str_preimm: instr=0x%08X\n", e->instr);

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t ea, new_base; bool wb;
    ctx_am2_addr_common(e, instr, base, &ea, &new_base, &wb);

    //log_printf("STR target EA=0x%08X <- r%u = 0x%08X\n", ea, rd, REG(rd));

    mem_write32(e->vm, ea, REG(rd));

    //log_printf("STR AFTER");

    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[STR pre-imm] [0x%08X] <= r%u (0x%08X) %s\n",
                   ea, rd, REG(rd), wb ? "(wb)" : "");
}

void handle_str_predec(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t rd    = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t ea, new_base; bool wb;
    ctx_am2_addr_common(e, instr, base, &ea, &new_base, &wb);

    mem_write32(e->vm, ea, REG(rd));
    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[STR pre-dec] [0x%08X] <= r%u (0x%08X) %s\n",
                   ea, rd, REG(rd), wb ? "(wb)" : "");
}

void handle_ldr_literal(exec_ctx_t *e) {
    uint32_t instr = e->instr;

    // imm12 and U bit
    uint32_t imm12 = instr & 0xFFFu;
    bool U = (instr >> 23) & 1u;

    // Read PC using the same semantics as any "register read" (i.e., PC+8 in ARM state),
    // then align to a word boundary (ARM literal base uses aligned PC).
    uint32_t pc_read = ctx_reg_read_src(e, 15);     // returns current_instr_addr + 8
    uint32_t base    = pc_read & ~3u;

    // Compute the literal pool address.
    uint32_t addr = U ? (base + imm12) : (base - imm12);

    // Destination register.
    uint32_t rd = (instr >> 12) & 0xFu;

    // Load the word via unified memory path.
    uint32_t val = mem_read32(e->vm, addr);

    // Writeback into Rd or PC (via npc) per architectural rules.
    if (rd == 15) {
        ctx_write_pc_via_npc(e, val);
        if (debug_flags & DBG_INSTR) {
            log_printf("[LDR lit] pc <= [0x%08X] => 0x%08X (npc)\n", addr, val);
        }
    } else {
        REG(rd) = val;
        if (debug_flags & DBG_INSTR) {
            log_printf("[LDR lit] r%u <= [0x%08X] => 0x%08X\n", rd, addr, val);

            // Optional extra visibility: dump the raw bytes we just read from.
            uint8_t b0 = mem_read8(e->vm, addr + 0);
            uint8_t b1 = mem_read8(e->vm, addr + 1);
            uint8_t b2 = mem_read8(e->vm, addr + 2);
            uint8_t b3 = mem_read8(e->vm, addr + 3);
            log_printf("[LDR lit] bytes @0x%08X = {%02X %02X %02X %02X}\n",
                       addr, b0, b1, b2, b3);
        }
    }
}

void handle_ldr_preimm(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t rd    = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t ea, new_base; bool wb;
    ctx_am2_addr_common(e, instr, base, &ea, &new_base, &wb);

    uint32_t val = mem_read32(e->vm, ea);

    if (rd == 15) {
        ctx_write_pc_via_npc(e, val);
    } else {
        REG(rd) = val;
    }

    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[LDR pre-imm] r%u <= [0x%08X] => 0x%08X %s\n",
                   rd, ea, val, wb ? "(wb)" : "");
}

void handle_ldr_postimm(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t rd    = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t ea, new_base; bool wb;
    ctx_am2_addr_common(e, instr, base, &ea, &new_base, &wb);

    uint32_t val = mem_read32(e->vm, ea);
    if (rd == 15) {
        ctx_write_pc_via_npc(e, val);
    } else {
        REG(rd) = val;
    }

    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[LDR post-imm] r%u <= [0x%08X] => 0x%08X %s\n",
                   rd, ea, val, wb ? "(wb)" : "");
}

void handle_ldrb_preimm(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t rd    = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t ea, new_base; bool wb;
    ctx_am2_addr_common(e, instr, base, &ea, &new_base, &wb);

    uint8_t val = mem_read8(e->vm, ea);
    REG(rd) = val;

    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[LDRB pre-imm] r%u <= [0x%08X] => 0x%02X %s\n",
                   rd, ea, val, wb ? "(wb)" : "");
}

void handle_ldrb_postimm(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t rd    = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t ea, new_base; bool wb;
    ctx_am2_addr_common(e, instr, base, &ea, &new_base, &wb);

    uint8_t val = mem_read8(e->vm, ea);
    REG(rd) = val;

    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[LDRB post-imm] r%u <= [0x%08X] => 0x%02X %s\n",
                   rd, ea, val, wb ? "(wb)" : "");
}

void handle_pop(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t regs  = instr & 0xBFFFu;  // mask out PC bit for pop() core

    uint32_t sp = REG(13);
    for (int i = 0; i < 16; ++i) {
        if (regs & (1u << i)) {
            REG(i) = mem_read32(e->vm, sp);
            sp += 4;
        }
    }
    REG(13) = sp;

    if (debug_flags & DBG_INSTR)
        log_printf("[POP] regs=0x%04X => new SP=0x%08X\n", regs, sp);
}

void handle_pop_pc(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t regs  = instr & 0x8000u;  // only PC bit

    uint32_t sp = REG(13);
    if (regs) {
        uint32_t val = mem_read32(e->vm, sp);
        sp += 4;
        ctx_write_pc_via_npc(e, val);
    }
    REG(13) = sp;

    if (debug_flags & DBG_INSTR)
        log_printf("[POP{..,pc}] new SP=0x%08X\n", sp);
}

void exec_ldrd_imm(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn = (instr >> 16) & 0xFu;
    uint32_t rd = (instr >> 12) & 0xFu;
    uint32_t rm = (instr >>  8) & 0xFu; (void)rm; // must be 0xF in ARMv5TE A32 enc

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t new_base; bool wb;
    uint32_t ea = ctx_extra_addr(e, instr, base, &new_base, &wb);

    uint32_t lo = mem_read32(e->vm, ea);
    uint32_t hi = mem_read32(e->vm, ea + 4);

    REG(rd)     = lo;
    REG(rd + 1) = hi;
    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[LDRD imm] r%u:r%u <= [0x%08X] => 0x%08X:0x%08X %s\n",
                   rd, rd+1, ea, lo, hi, wb ? "(wb)" : "");
}

void exec_ldrd_reg(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn = (instr >> 16) & 0xFu;
    uint32_t rd = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t new_base; bool wb;
    uint32_t ea = ctx_extra_addr(e, instr, base, &new_base, &wb);

    uint32_t lo = mem_read32(e->vm, ea);
    uint32_t hi = mem_read32(e->vm, ea + 4);

    REG(rd)     = lo;
    REG(rd + 1) = hi;
    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[LDRD reg] r%u:r%u <= [0x%08X] => 0x%08X:0x%08X %s\n",
                   rd, rd+1, ea, lo, hi, wb ? "(wb)" : "");
}

void exec_strd_imm(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn = (instr >> 16) & 0xFu;
    uint32_t rd = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t new_base; bool wb;
    uint32_t ea = ctx_extra_addr(e, instr, base, &new_base, &wb);

    uint32_t lo = REG(rd);
    uint32_t hi = REG(rd + 1);
    mem_write32(e->vm, ea,     lo);
    mem_write32(e->vm, ea + 4, hi);

    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[STRD imm] [0x%08X] <= r%u:r%u (0x%08X:0x%08X) %s\n",
                   ea, rd, rd+1, lo, hi, wb ? "(wb)" : "");
}

void exec_strd_reg(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn = (instr >> 16) & 0xFu;
    uint32_t rd = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t new_base; bool wb;
    uint32_t ea = ctx_extra_addr(e, instr, base, &new_base, &wb);

    uint32_t lo = REG(rd);
    uint32_t hi = REG(rd + 1);
    mem_write32(e->vm, ea,     lo);
    mem_write32(e->vm, ea + 4, hi);

    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[STRD reg] [0x%08X] <= r%u:r%u (0x%08X:0x%08X) %s\n",
                   ea, rd, rd+1, lo, hi, wb ? "(wb)" : "");
}

void handle_ldr_regoffset(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t rd    = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t ea = base + ctx_am2_reg_scaled(e, instr);

    uint32_t val = mem_read32(e->vm, ea);
    if (rd == 15)
        ctx_write_pc_via_npc(e, val);
    else
        REG(rd) = val;

    if (debug_flags & DBG_INSTR)
        log_printf("[LDR reg] r%u <= [0x%08X] => 0x%08X\n", rd, ea, val);
}

void handle_ldrb_reg_shift(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t rd    = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t ea = base + ctx_am2_reg_scaled(e, instr);

    uint8_t val = mem_read8(e->vm, ea);
    REG(rd) = val;

    if (debug_flags & DBG_INSTR)
        log_printf("[LDRB reg shift] r%u <= [0x%08X] => 0x%02X\n",
                   rd, ea, val);
}

void handle_strh(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn = (instr >> 16) & 0xFu;
    uint32_t rd = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t new_base; bool wb;
    uint32_t ea = ctx_extra_addr(e, instr, base, &new_base, &wb);

    uint16_t v = (uint16_t)REG(rd);
    mem_write8(e->vm, ea,     (uint8_t)(v & 0xFFu));
    mem_write8(e->vm, ea + 1, (uint8_t)(v >> 8));

    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[STRH] [0x%08X] <= r%u (0x%04X) %s\n",
                   ea, rd, v, wb ? "(wb)" : "");
}

void handle_ldrh(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn = (instr >> 16) & 0xFu;
    uint32_t rd = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t new_base; bool wb;
    uint32_t ea = ctx_extra_addr(e, instr, base, &new_base, &wb);

    uint16_t v = (uint16_t)mem_read8(e->vm, ea)
               | ((uint16_t)mem_read8(e->vm, ea + 1) << 8);
    REG(rd) = v;

    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[LDRH] r%u <= [0x%08X] => 0x%04X %s\n",
                   rd, ea, v, wb ? "(wb)" : "");
}

void handle_ldrsb(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn = (instr >> 16) & 0xFu;
    uint32_t rd = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t new_base; bool wb;
    uint32_t ea = ctx_extra_addr(e, instr, base, &new_base, &wb);

    int8_t  v = (int8_t)mem_read8(e->vm, ea);
    REG(rd) = (uint32_t)(int32_t)v;

    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[LDRSB] r%u <= [0x%08X] => %d %s\n",
                   rd, ea, (int)v, wb ? "(wb)" : "");
}

void handle_ldrsh(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn = (instr >> 16) & 0xFu;
    uint32_t rd = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t new_base; bool wb;
    uint32_t ea = ctx_extra_addr(e, instr, base, &new_base, &wb);

    int16_t v = (int16_t)(
        (uint16_t)mem_read8(e->vm, ea) |
        ((uint16_t)mem_read8(e->vm, ea + 1) << 8)
    );
    REG(rd) = (uint32_t)(int32_t)v;

    if (wb) REG(rn) = new_base;

    if (debug_flags & DBG_INSTR)
        log_printf("[LDRSH] r%u <= [0x%08X] => %d %s\n",
                   rd, ea, (int)v, wb ? "(wb)" : "");
}

void handle_ldm(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t w     = (instr >> 21) & 1u;
    uint32_t U     = (instr >> 23) & 1u;
    uint32_t P     = (instr >> 24) & 1u;
    uint32_t regs  = instr & 0xFFFFu;

    uint32_t base = REG(rn);
    unsigned count = 0;
    for (unsigned i = 0; i < 16; ++i)
        if (regs & (1u << i)) ++count;

    uint32_t addr;
    if (U)
        addr = base + (P ? 4u : 0u);
    else
        addr = base - 4u * count + (P ? 0u : 4u);

    uint32_t first = addr;
    for (unsigned i = 0; i < 16; ++i) {
        if (regs & (1u << i)) {
            uint32_t v = mem_read32(e->vm, addr);
            if (i == 15)
                ctx_write_pc_via_npc(e, v);
            else
                REG(i) = v;
            addr += 4u;
        }
    }

    if (w)
        REG(rn) = U ? (base + 4u * count) : (base - 4u * count);

    if (debug_flags & DBG_INSTR)
        log_printf("[LDM] r%u base=0x%08X regs=0x%04X first=0x%08X => wb=0x%08X\n",
                   rn, base, regs, first, REG(rn));
}

void handle_str_regoffset(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t rd    = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t ea = base + ctx_am2_reg_scaled(e, instr);

    mem_write32(e->vm, ea, REG(rd));

    if (debug_flags & DBG_INSTR)
        log_printf("[STR reg] [0x%08X] <= r%u (0x%08X)\n",
                   ea, rd, REG(rd));
}

void handle_strb_reg_shift(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t rd    = (instr >> 12) & 0xFu;

    uint32_t base = ctx_reg_read_src(e, rn);
    uint32_t ea = base + ctx_am2_reg_scaled(e, instr);

    mem_write8(e->vm, ea, (uint8_t)REG(rd));

    if (debug_flags & DBG_INSTR)
        log_printf("[STRB reg shift] [0x%08X] <= r%u (0x%08X)\n",
                   ea, rd, REG(rd));
}

void handle_swp(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn = (instr >> 16) & 0xFu;
    uint32_t rd = (instr >> 12) & 0xFu;
    uint32_t rm = (instr      ) & 0xFu;

    uint32_t addr = REG(rn);
    uint32_t tmp  = REG(rm);
    uint32_t memv = mem_read32(e->vm, addr);
    mem_write32(e->vm, addr, tmp);
    REG(rd) = memv;

    if (debug_flags & DBG_INSTR)
        log_printf("[SWP] r%u <=> [0x%08X] (r%u) => 0x%08X\n",
                   rd, addr, rm, memv);
}

void handle_swpb(exec_ctx_t *e){
    uint32_t instr = e->instr;
    uint32_t rn = (instr >> 16) & 0xFu;
    uint32_t rd = (instr >> 12) & 0xFu;
    uint32_t rm = (instr      ) & 0xFu;

    uint32_t addr = REG(rn);
    uint8_t  tmp  = (uint8_t)REG(rm);
    uint8_t  memv = mem_read8(e->vm, addr);
    mem_write8(e->vm, addr, tmp);
    REG(rd) = memv;

    if (debug_flags & DBG_INSTR)
        log_printf("[SWPB] r%u <=> [0x%08X] (r%u) => 0x%02X\n",
                   rd, addr, rm, memv);
}
