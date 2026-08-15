// src/include/dev_nvram.h (new)
#pragma once
#include <stdint.h>

#define NVRAM_BASE_ADDR  0xF0003000u
#define NVRAM_MMIO_SIZE  0x100u

// Simple map: [0x00] INDEX (8-bit), [0x04] DATA (8-bit in low byte)
#define NVRAM_REG_INDEX  0x00u
#define NVRAM_REG_DATA   0x04u

// Define a few well-known offsets
#define NVRAM_OFF_BOOT_ORDER  0x00u   // 0:DISK, 1:NET, 2:CD (your choice)
#define NVRAM_CAPACITY        256u    // 256 byte blob

void     dev_nvram_init(const char *backing_path); // e.g. "nvram.bin"
uint32_t dev_nvram_read32(uint32_t addr);
void     dev_nvram_write32(uint32_t addr, uint32_t value);