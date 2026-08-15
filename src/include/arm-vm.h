#ifndef ARM_VM_H
#define ARM_VM_H
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "cpu.h"   // brings in typedef CPU and extern CPU cpu;

#ifndef BIT
#define BIT(x) (1u << (x))
#endif

// ---------------------------

/* Optional helpers */
void crt_refresh(void);
void crt_mark_dirty(uint32_t addr, size_t len);

extern uint8_t *ram;      // allocated at runtime
extern size_t   ram_size; // bytes

// ---------- UART ----------
#define UART_BASE   0x09000000u
#define UART_DR     0x00u

// ---------- CRT ----------
#define CRT_BASE    0x0A000000u
#define CRT_COLS    80
#define CRT_ROWS    25
#define CRT_SIZE    (CRT_ROWS * CRT_COLS * 2)

// ---------- DISK (MMIO device) ----------
#define DISK_BASE         0x0B000000u
#define DISK_MMIO_SIZE    0x1000u
#define DISK_SECTOR_SIZE  512u
#define DISK_NUM_SECTORS  256u  // 128 KiB example image
#define DISK_SIZE         DISK_MMIO_SIZE  // keep arm-vm.c happy

// Register offsets from DISK_BASE
#define DISK_REG_CMD       0x00u  // write 1 = READ; write 0 = reset/idle
#define DISK_REG_STATUS    0x04u  // 0=idle, 1=busy, 2=error, 3=done
#define DISK_REG_LBA       0x08u  // sector index
#define DISK_REG_BUF_ADDR  0x0Cu  // guest physical address (DMA target)

// ---------- Shared flags / globals ----------
extern bool trace_all;
extern bool enable_crt;
extern bool crt_dirty;

// Backing store for the disk image (defined in disk.c)
extern uint8_t disk_data[DISK_NUM_SECTORS * DISK_SECTOR_SIZE];

// ---------- MMIO handlers ----------
uint32_t handle_uart_read  (uint32_t addr);
void     handle_uart_write (uint32_t addr, uint32_t value);

uint32_t handle_crt_read   (uint32_t addr);
void     handle_crt_write  (uint32_t addr, uint32_t value);

uint32_t handle_disk_read  (uint32_t addr);
void     handle_disk_write (uint32_t addr, uint32_t value);

// ---------- Utilities ----------
bool  load_disk_image(const char *path);
void  log_info (const char *fmt, ...);
void  log_warn (const char *fmt, ...);
void  log_error(const char *fmt, ...);

#endif // ARM_VM_H
