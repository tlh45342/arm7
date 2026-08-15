// src/vm.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <inttypes.h>

#include "vm.h"       // public VM API (opaque VM, fn typedefs, debug_flags_t)
#include "cpu.h"      // CPU type, execute(uint32_t instr)
#include "mem.h"      // mem_read32(uint32_t addr)
#include "log.h"      // log_printf(...)
#include "debug.h"    // debug_flags, DBG_INSTR (flags type: debug_flags_t)
#include "dev_uart.h"
#include "dev_keyboard.h"
#include "dev_rtc.h"

/* --------------------------------------------------------------------------------
   Internal VM layout (vm.h keeps VM opaque; we define it here)
-------------------------------------------------------------------------------- */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif


bool arm_condition_holds(const exec_ctx_t *e, uint32_t cond) {
    const CPU *ctx = &e->vm->cpu;
    unsigned Z = (ctx->cpsr & CPSR_Z) ? 1u : 0u;
    unsigned C = (ctx->cpsr & CPSR_C) ? 1u : 0u;
    unsigned N = (ctx->cpsr & CPSR_N) ? 1u : 0u;
    unsigned V = (ctx->cpsr & CPSR_V) ? 1u : 0u;

    switch (cond & 0xFu) {
        case COND_EQ: return  Z;
        case COND_NE: return !Z;
        case COND_CS: return  C;
        case COND_CC: return !C;
        case COND_MI: return  N;
        case COND_PL: return !N;
        case COND_VS: return  V;
        case COND_VC: return !V;
        case COND_HI: return  C && !Z;
        case COND_LS: return !C ||  Z;
        case COND_GE: return  N == V;
        case COND_LT: return  N != V;
        case COND_GT: return !Z && (N == V);
        case COND_LE: return  Z || (N != V);
        case COND_AL: return true;
        case COND_NV: default: return false;
    }
}

/* --------------------------------------------------------------------------------
   Lifetime
-------------------------------------------------------------------------------- */

VM* vm_create(void) {
    VM *vm = (VM*)calloc(1, sizeof(*vm));
    return vm; /* Attach RAM later via vm_add_ram() */
}

void vm_destroy(VM* vm) {
    if (!vm) return;
    free(vm->ram);
    free(vm);
}

void vm_reset(VM* vm) {
    if (!vm) return;
    memset(&vm->cpu, 0, sizeof(vm->cpu));
    vm->halted = false;
    dev_keyboard_reset();
}

/* Allocate/attach RAM (size_t per vm.h), store size clamped to 32-bit field */
bool vm_add_ram(VM* vm, size_t ram_size) {
    if (!vm) return false;
    if (vm->ram) { free(vm->ram); vm->ram = NULL; vm->ram_size = 0; }
    vm->ram = (uint8_t*)calloc(1, ram_size);
    if (!vm->ram) return false;
    vm->ram_size = (uint32_t)ram_size;
    return true;
}

/* --------------------------------------------------------------------------------
   RAM helpers
-------------------------------------------------------------------------------- */

uint8_t* vm_ram_ptr(VM* vm, uint32_t addr) {
    if (!vm || !vm->ram) return NULL;
    if (addr >= vm->ram_size) return NULL;
    return vm->ram + addr;
}

// Minimal RAM bounds check just for RAM (no MMIO here yet)
static inline bool vm_addr_ok(const VM *vm, uint32_t addr, size_t len) {
    if (!vm || !vm->ram) return false;
    if (addr >= vm->ram_size) return false;
    if (len > (size_t)(vm->ram_size - addr)) return false;
    return true;
}

bool vm_write_mem8(struct VM *vm, uint32_t addr, uint8_t v) {
    if (!vm_addr_ok(vm, addr, 1)) return false;
    vm->ram[addr] = v;
    return true;
}

bool vm_poke(VM* vm, uint32_t addr, uint32_t value) {
    if (!vm || !vm->ram) return false;
    if (addr + 4 > vm->ram_size) return false;
    *(uint32_t*)(vm->ram + addr) = value;
    return true;
}

bool vm_peek(const VM* vm, uint32_t addr, uint32_t* out) {
    if (!vm || !vm->ram || !out) return false;
    if (addr + 4 > vm->ram_size) return false;
    *out = *(const uint32_t*)(vm->ram + addr);
    return true;
}

/* vm_read_mem as declared in vm.h: copy out len bytes, return success */
bool vm_read_mem(VM* vm, uint32_t addr, void* out, size_t len) {
    if (!vm || !out || !vm->ram) return false;
    if (addr > vm->ram_size) return false;
    if (len > (size_t)(vm->ram_size - addr)) return false;
    memcpy(out, vm->ram + addr, len);
    return true;
}

// Reads a 32-bit value from VM memory at the given address
bool vm_read_mem32(VM* vm, uint32_t addr, uint32_t* out) {
    if (!out) return false;
    uint8_t buffer[4];
    if (!vm_read_mem(vm, addr, buffer, sizeof(buffer))) return false;

    // Assuming little-endian memory layout
    *out = (uint32_t)buffer[0] |
           ((uint32_t)buffer[1] << 8) |
           ((uint32_t)buffer[2] << 16) |
           ((uint32_t)buffer[3] << 24);
    return true;
}

void vm_devices_init(VM *vm)
{
    (void)vm; // in case you don't need it yet

    // Wire UART0 at 0x09000000
    dev_uart_init(0x09000000u);
    dev_keyboard_init(KBD_BASE_ADDR);
    dev_rtc_init(RTC_BASE_ADDR);
}

// Reads the instruction at the current PC
uint32_t vm_read_instr(VM* vm) {
    uint32_t pc = vm_get_reg(vm, 15);
    uint32_t instr = 0;
    vm_read_mem32(vm, pc, &instr);
    return instr;
}

void vm_devices_tick(VM *vm, uint32_t cycles)
{
    (void)vm;
    (void)cycles;
    // Future: advance disk state, timers, interrupts, etc.
}

bool vm_keyboard_push(VM *vm, uint8_t ch) {
    (void)vm;
    return dev_keyboard_push(ch);
}
unsigned vm_keyboard_pending(const VM *vm) {
    (void)vm;
    return dev_keyboard_count();
}

void vm_clear_halt(VM* vm) {
    if (!vm) return;
    vm->halted = false;
}

/* CLI helper expected by callers */
void vm_dump_regs(VM *vm) {
    if (!vm) return;
    for (int i = 0; i < 16; ++i) {
        log_printf("r%-2d = 0x%08X%s",
                   i, vm_get_reg(vm, i),
                   (i % 4 == 3) ? "\n" : "  ");
    }
    log_printf("CPSR = 0x%08X  cycle=%llu\n", vm->cpu.cpsr, (unsigned long long)vm->cpu.cycles);
}

/* Per vm.h: include VM* and debug_flags_t in signature */
void vm_set_debug(VM *vm, debug_flags_t flags) {
    (void)vm;            /* not used presently */
    debug_flags = flags; /* global from debug.h */
}

/* --------------------------------------------------------------------------------
   Registers (names match callers per link errors)
-------------------------------------------------------------------------------- */

uint32_t vm_get_reg(const VM* vm, int idx) {
    if (!vm || idx < 0 || idx >= 16) return 0;
    return vm->cpu.r[idx];
}

void vm_set_reg(VM* vm, int idx, uint32_t value) {
    if (!vm || idx < 0 || idx >= 16) return;
    vm->cpu.r[idx] = value;

    if (idx == 15) {
        vm->cpu.npc = value & ~3u;  // word align
    }
}

/* --------------------------------------------------------------------------------
   MMIO registry (pure C11, no typeof)
-------------------------------------------------------------------------------- */

bool vm_map_mmio(VM* vm, uint32_t base, uint32_t size,
                 vm_mmio_read_fn rfn, vm_mmio_write_fn wfn, void* ctx)
{
    if (!vm) return false;
    if (vm->n_mmio < 0 || (size_t)vm->n_mmio >= ARRAY_SIZE(vm->mmio)) {
        return false; /* table full */
    }
    const int i = vm->n_mmio++;
    vm->mmio[i].base = base;
    vm->mmio[i].size = size;
    vm->mmio[i].rfn  = rfn;
    vm->mmio[i].wfn  = wfn;
    vm->mmio[i].ctx  = ctx;
    return true;
}

/* --------------------------------------------------------------------------------
   Loader
-------------------------------------------------------------------------------- */

bool vm_load_binary(VM *vm, const char *path, uint32_t addr) {
    if (!vm || !path) return false;

    FILE *f = fopen(path, "rb");
    if (!f) return false;

    uint8_t buf[4096];
    size_t nread, total = 0;
    uint32_t p = addr;

    while ((nread = fread(buf, 1, sizeof buf, f)) > 0) {
        for (size_t i = 0; i < nread; ++i) {
            if (!vm_write_mem8(vm, p++, buf[i])) {
                fclose(f);
                return false;  // write fault
            }
        }
        total += nread;
    }
    fclose(f);

    // (Optional) seed entry PC here if you want:
    // vm->cpu.r[15] = addr;
    // vm->cpu.npc   = addr & ~3u;

    log_printf("[LOAD] %s @ 0x%08X (%zu bytes)\n", path, addr, total);
    return true;
}

/* --------------------------------------------------------------------------------
   Run / step / debug
-------------------------------------------------------------------------------- */

bool vm_run(VM* vm, uint64_t max_cycles) {
    if (!vm) return false;

    uint64_t cycles = 0;
    while (!vm->halted && (max_cycles == 0 || cycles < max_cycles)) {
        uint32_t pc = vm->cpu.r[15];
        uint32_t instr = 0;

        if (!vm_read_mem32(vm, pc, &instr)) {
            vm->halted = true;
            break;
        }

        exec_ctx_t e;
        make_ctx(&e, vm, pc, instr);

        cpu_execute(&e);

		 vm_devices_tick(vm, 1);
		 
        if (vm->cpu.halted) vm->halted = true;
        ++cycles;
    }
    return !vm->halted;
}

/* One-instruction step (used by CLI) */
bool vm_step(VM* vm) {
    if (!vm || vm->halted) return false;

    uint32_t pc = vm->cpu.r[15];
    uint32_t instr = 0;

    if (!vm_read_mem32(vm, pc, &instr)) {
        vm->halted = true;
        return false;
    }

    exec_ctx_t e;
    make_ctx(&e, vm, pc, instr);

    cpu_execute(&e);

    vm_devices_tick(vm, 1);
	 
    if (vm->cpu.halted) vm->halted = true;
    return !vm->halted;
}