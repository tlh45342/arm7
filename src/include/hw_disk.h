// include/hw_disk.h
#pragma once
#include "hw.h"
#include <stdio.h>

typedef struct {
    // MMIO regs
    uint32_t cmd, lba, buf, status;
    // tick state
    int      remaining;
    bool     pending;
    // backing image
    FILE*    img;
    size_t   img_sz;
} DiskState;

bool hw_disk_create(HWDevice* out, uint32_t base, const char* img_path);
