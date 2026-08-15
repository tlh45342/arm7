// disk_manager.h
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef MAX_DISKS
#define MAX_DISKS 4
#endif

#ifndef SECTOR_SIZE
#define SECTOR_SIZE 512u
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool     present;
    bool     readonly;
    char     path[260];
    uint8_t *img;          // entire image in host RAM
    size_t   size_bytes;
} DiskSlot;

void   disk_init(void);
bool   disk_attach(int slot, const char *path, bool readonly);
bool   disk_detach(int slot);

bool   disk_present(int slot);
bool   disk_read_sectors(int slot, uint64_t lba, void *dst, uint32_t nsec);
bool   disk_write_sectors(int slot, uint64_t lba, const void *src, uint32_t nsec); // respects readonly

size_t disk_size_bytes(int slot);
size_t disk_num_sectors(int slot);

// CLI / presentation helpers
void   disk_print_list(void);
void   disk_print_info(int slot);

// Access to the slots (if you need it for MMIO)
const DiskSlot *disk_get_slot(int slot);

#ifdef __cplusplus
}
#endif