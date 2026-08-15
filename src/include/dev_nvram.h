#pragma once
#include <stdint.h>

#define NVRAM_BASE_ADDR  0xF0003000u
#define NVRAM_MMIO_SIZE  0x100u

// Simple “index/data” register pair (PC-style, but MMIO addresses)
#define NVRAM_REG_INDEX  0x00u
#define NVRAM_REG_DATA   0x04u

// Byte offsets inside the NVRAM blob
#define NVRAM_OFF_BOOT_ORDER  0x00u  // 0:DISK, 1:NET, 2:CD (tweak as you like)
#define NVRAM_CAPACITY        256u

void     dev_nvram_init(const char *backing_path);
uint32_t dev_nvram_read32(uint32_t addr);
void     dev_nvram_write32(uint32_t addr, uint32_t value);