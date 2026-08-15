// src/include/dev_disk.h
#ifndef DEV_DISK_H
#define DEV_DISK_H

#include <stdbool.h>
#include <stdint.h>
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DISK0_BASE 0x0B000000u

/*
 * disk0 PIO MMIO contract
 *
 *   +0x04  LBA       read/write
 *   +0x08  COUNT     read/write
 *   +0x0C  CMD       write (READ=0x01, GO=0x80)
 *   +0x10  STATUS    read (BUSY=0x01, DRQ=0x02, ERR=0x04)
 *   +0x200 DATA      512-byte read window
 *
 * Current implementation services one sector per command.
 */
bool     dev_disk0_attach(VM *vm, const char *image_path);
bool     dev_disk0_present(void);
uint32_t dev_disk0_read_reg(uint32_t addr);
void     dev_disk0_write_reg(uint32_t addr, uint32_t val);

#ifdef __cplusplus
}
#endif

#endif
