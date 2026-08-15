// src/mem.c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "vm.h"
#include "mem.h"
#include "dev_disk.h"   // dev_disk0_present(), dev_disk0_read_reg(), dev_disk0_write_reg()
#include "dev_uart.h"   // dev_uart_present(), dev_uart_read_reg(), dev_uart_write_reg()
#include "dev_keyboard.h"
#include "dev_rtc.h"

// ==========================
// MMIO windows (word-based)
// ==========================
#define DISK0_BASE 0x0B000000u
#define DISK0_SIZE 0x00001000u

#define UART0_BASE 0x09000000u
#define UART0_SIZE 0x00001000u

#define KBD_BASE   KBD_BASE_ADDR
#define KBD_SIZE   KBD_MMIO_SIZE

static inline bool in_disk0(uint32_t addr) {
    return (uint32_t)(addr - DISK0_BASE) < DISK0_SIZE;
}

static inline bool in_uart0(uint32_t addr) {
    return (uint32_t)(addr - UART0_BASE) < UART0_SIZE;
}

static inline bool in_keyboard(uint32_t addr) {
    return (uint32_t)(addr - KBD_BASE) < KBD_SIZE;
}

#define RTC_BASE RTC_BASE_ADDR
#define RTC_SIZE RTC_MMIO_SIZE

static inline bool in_rtc(uint32_t addr) {
    return (uint32_t)(addr - RTC_BASE) < RTC_SIZE;
}

// ==========================
// RAM range check
// ==========================

bool mem_in_range(VM *vm, uint32_t addr, size_t size)
{
    if (!vm || !vm->ram) {
        return false;
    }

    // Treat guest physical address as offset into vm->ram.
    // We assume vm->ram_size is the total RAM size in bytes.
    uint64_t end = (uint64_t)addr + (uint64_t)size;
    return end <= (uint64_t)vm->ram_size;
}

// ==========================
// 8-bit access (RAM only)
// ==========================

uint8_t mem_read8(VM *vm, uint32_t addr)
{
    if (!mem_in_range(vm, addr, 1)) {
        // TODO: data abort / log
        return 0;
    }

    return vm->ram[addr];
}

void mem_write8(VM *vm, uint32_t addr, uint8_t value)
{
    if (!mem_in_range(vm, addr, 1)) {
        // TODO: data abort / log
        return;
    }

    vm->ram[addr] = value;
}

// ==========================
// 16-bit access (RAM only)
// ==========================

uint16_t mem_read16(VM *vm, uint32_t addr)
{
    if (!mem_in_range(vm, addr, 2)) {
        // TODO: data abort / log
        return 0;
    }

    uint8_t *m = vm->ram + addr;

    return (uint16_t)( (uint16_t)m[0]
                     | ((uint16_t)m[1] << 8) );
}

void mem_write16(VM *vm, uint32_t addr, uint16_t value)
{
    if (!mem_in_range(vm, addr, 2)) {
        // TODO: data abort / log
        return;
    }

    uint8_t *m = vm->ram + addr;

    m[0] = (uint8_t)( value        & 0xFFu );
    m[1] = (uint8_t)((value >>  8) & 0xFFu );
}

// ==========================
// 32-bit access (RAM + MMIO)
// ==========================

uint32_t mem_read32(VM *vm, uint32_t addr)
{
    // -------- MMIO: RTC --------
    if (in_rtc(addr)) {
        return dev_rtc_read32(addr & ~0x3u);
    }

    // -------- MMIO: KEYBOARD --------
    if (in_keyboard(addr)) {
        uint32_t aligned = addr & ~0x3u;
        return dev_keyboard_read32(aligned);
    }

    // -------- MMIO: UART0 --------
    if (in_uart0(addr) && dev_uart_present()) {
        // Device code interprets full address; it subtracts base internally.
        // We assume word-aligned semantics for MMIO.
        uint32_t aligned = addr & ~0x3u;
        return dev_uart_read_reg(aligned);
    }

    // -------- MMIO: DISK0 --------
    if (in_disk0(addr) && dev_disk0_present()) {
        uint32_t aligned = addr & ~0x3u;
        return dev_disk0_read_reg(aligned);
    }

    // -------- Normal RAM --------
    if (!mem_in_range(vm, addr, 4)) {
        // TODO: data abort / log
        return 0;
    }

    uint8_t *m = vm->ram + addr;

    return  (uint32_t)m[0]
         | ((uint32_t)m[1] << 8)
         | ((uint32_t)m[2] << 16)
         | ((uint32_t)m[3] << 24);
}

void mem_write32(VM *vm, uint32_t addr, uint32_t value)
{
    // -------- MMIO: RTC --------
    if (in_rtc(addr)) {
        dev_rtc_write32(addr & ~0x3u, value);
        return;
    }

    // -------- MMIO: KEYBOARD --------
    if (in_keyboard(addr)) {
        uint32_t aligned = addr & ~0x3u;
        dev_keyboard_write32(aligned, value);
        return;
    }

    // -------- MMIO: UART0 --------
    if (in_uart0(addr) && dev_uart_present()) {
        uint32_t aligned = addr & ~0x3u;
        dev_uart_write_reg(aligned, value);
        return;
    }

    // -------- MMIO: DISK0 --------
    if (in_disk0(addr) && dev_disk0_present()) {
        uint32_t aligned = addr & ~0x3u;
        dev_disk0_write_reg(aligned, value);
        return;
    }

    // -------- Normal RAM --------
    if (!mem_in_range(vm, addr, 4)) {
        // TODO: data abort / log
        return;
    }

    uint8_t *m = vm->ram + addr;

    //log_printf("[mem_write32] vm->ram=%p base addr=%p\n", vm->ram, vm->ram + addr);

    m[0] = (uint8_t)( value        & 0xFFu );
    m[1] = (uint8_t)((value >>  8) & 0xFFu );
    m[2] = (uint8_t)((value >> 16) & 0xFFu );
    m[3] = (uint8_t)((value >> 24) & 0xFFu );

    //log_printf("[mem_write32] POST\n");
}
