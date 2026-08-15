// src/include/vm.h
#ifndef VM_H
#define VM_H
/* #pragma once */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "cpu.h"      // brings CPU and halt_reason_t
#include "debug.h"    // debug_flags_t

typedef debug_flags_t vm_debug_t;

/* -------- MMIO callback types -------- */
typedef uint32_t (*vm_mmio_read_fn)(void* ctx, uint32_t addr);
typedef void     (*vm_mmio_write_fn)(void* ctx, uint32_t addr, uint32_t value);

/* -------- MMIO entry -------- */
typedef struct vm_mmio_entry {
    uint32_t          base;
    uint32_t          size;
    vm_mmio_read_fn   rfn;
    vm_mmio_write_fn  wfn;
    void             *ctx;
} vm_mmio_entry;

/* -------- VM struct -------- */
typedef struct VM {
    uint8_t       *ram;            /* base of RAM */
    uint32_t       ram_size;       /* size (bytes, clamped to 32-bit) */
    CPU            cpu;            /* core state */
    vm_mmio_entry  mmio[16];       /* MMIO registry */
    int            n_mmio;         /* number of active MMIO entries */
    bool           halted;         /* run/step state */
	uint64_t       cycles;         /* 11-28-2025 */
} VM;

/* -------- VM run state -------- */
typedef enum { VM_STOPPED, VM_RUNNING, VM_HALTED } vm_state_t;

/* -------- Lifecycle -------- */
VM*             vm_create(void);
void            vm_devices_init(VM *vm);
bool            vm_add_ram(VM* vm, size_t ram_size);
size_t          vm_ram_size(const VM* vm);
void            vm_reset(VM* vm);
void            vm_destroy(VM* vm);
void            vm_set_debug(VM *vm, debug_flags_t flags);
debug_flags_t   vm_get_debug(const VM *vm);

/* -------- Execution -------- */
bool            vm_step(VM* vm);
bool            vm_run(VM* vm, uint64_t max_cycles);   // 0 = run until halt
void            vm_halt(VM* vm);
bool            vm_is_halted(const VM* vm);
void            vm_clear_halt(VM *vm);

/* -------- Memory convenience -------- */
bool             vm_load_binary(VM* vm, const char* path, uint32_t addr);
bool             vm_read_mem(VM* vm, uint32_t addr, void* out, size_t len);
bool             vm_write_mem(VM* vm, uint32_t addr, const void* in, size_t len);
bool             vm_write_mem8(VM *vm, uint32_t addr, uint8_t v);

/* Host -> guest keyboard FIFO */
bool             vm_keyboard_push(VM *vm, uint8_t ch);
unsigned         vm_keyboard_pending(const VM *vm);
bool             vm_read_mem32(VM *vm, uint32_t addr, uint32_t *out);
uint8_t          vm_read8(VM *vm, uint32_t addr);

/* -------- Registers -------- */
uint32_t         vm_get_reg(const VM* vm, int idx);    // 0..15
void             vm_set_reg(VM* vm, int idx, uint32_t value);
uint32_t         vm_get_cpsr(const VM* vm);
void             vm_set_cpsr(VM* vm, uint32_t value);
void             vm_dump_regs(VM *vm);

/* -------- MMIO callback registration -------- */
bool             vm_map_mmio(VM* vm, uint32_t base, uint32_t size,
                             vm_mmio_read_fn rfn, vm_mmio_write_fn wfn,
                             void* ctx);

#endif // VM_H
