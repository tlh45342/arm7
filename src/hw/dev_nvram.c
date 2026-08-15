// src/hw/dev_nvram.c (new)
#include <stdio.h>
#include <string.h>
#include "dev_nvram.h"
#include "log.h"

static uint8_t nvram[NVRAM_CAPACITY];
static uint8_t index_reg = 0;
static char    nvram_path[260];

static void nvram_load(void) {
    FILE *f = fopen(nvram_path, "rb");
    if (f) {
        fread(nvram, 1, NVRAM_CAPACITY, f);
        fclose(f);
    } else {
        memset(nvram, 0, sizeof nvram);
        // default boot order: DISK
        nvram[NVRAM_OFF_BOOT_ORDER] = 0; // 0:DISK, 1:NET, 2:CD
    }
}
static void nvram_save(void) {
    FILE *f = fopen(nvram_path, "wb");
    if (f) {
        fwrite(nvram, 1, NVRAM_CAPACITY, f);
        fclose(f);
    }
}

void dev_nvram_init(const char *backing_path) {
    strncpy(nvram_path, backing_path ? backing_path : "nvram.bin", sizeof nvram_path);
    nvram_path[sizeof nvram_path - 1] = '\0';
    nvram_load();
}

uint32_t dev_nvram_read32(uint32_t addr) {
    uint32_t off = addr - NVRAM_BASE_ADDR;
    switch (off) {
    case NVRAM_REG_INDEX: return index_reg; // low byte used
    case NVRAM_REG_DATA:  return nvram[index_reg];
    default:              return 0;
    }
}

void dev_nvram_write32(uint32_t addr, uint32_t value) {
    uint32_t off = addr - NVRAM_BASE_ADDR;
    switch (off) {
    case NVRAM_REG_INDEX:
        index_reg = (uint8_t)(value & 0xFF);
        break;
    case NVRAM_REG_DATA:
        nvram[index_reg] = (uint8_t)(value & 0xFF);
        nvram_save();
        break;
    default:
        break;
    }
}