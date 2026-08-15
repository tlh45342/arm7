// src/dev_disk.c
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "dev_disk.h"      // must define DISK0_BASE and VM/vm_map_mmio types
#include "disk_manager.h"  // disk_init, disk_attach, disk_present, disk_read_sectors, disk_size_bytes
#include "log.h"

#define SECTOR_SIZE 512u

// MMIO layout (keep regs the same, move DATA away from them)
enum {
    REG_LBA    = 0x04,
    REG_COUNT  = 0x08,
    REG_CMD    = 0x0C,
    REG_STATUS = 0x10,
    REG_DATA   = 0x200,           // 512-byte window at +0x200
};

// Status bits
enum {
    ST_BUSY = 0x01,
    ST_DRQ  = 0x02,
    ST_ERR  = 0x04,
};

// Command bits
enum {
    CMD_READ = 0x01,
    CMD_GO   = 0x80,
};

typedef struct {
    VM       *vm;
    uint32_t base;

    // control
    uint32_t lba;
    uint32_t count;                // sectors requested (we service at least 1)
    uint32_t status;

    // data buffer exposed at REG_DATA..+511
    uint8_t  data[SECTOR_SIZE];

    // backing image
    int      slot;                 // disk_manager slot
    bool     mapped;
} DiskCtl;

static DiskCtl g_disk0 = {0};

static uint32_t disk_mmio_read(void *ctx, uint32_t addr) {
    DiskCtl *d = (DiskCtl*)ctx;
    const uint32_t off = addr - d->base;

    switch (off) {
        case REG_CMD:    return 0;          // write-only
        case REG_LBA:    return d->lba;
        case REG_COUNT:  return d->count;
        case REG_STATUS: return d->status;

        default:
            // Present the 512-byte data window as 32-bit little-endian words.
            // Support unaligned LDRs by assembling from up to 4 bytes.
            if (off >= REG_DATA && off - REG_DATA < SECTOR_SIZE) {
                uint32_t i  = off - REG_DATA;
                uint32_t b0 = (i + 0 < SECTOR_SIZE) ? d->data[i + 0] : 0;
                uint32_t b1 = (i + 1 < SECTOR_SIZE) ? d->data[i + 1] : 0;
                uint32_t b2 = (i + 2 < SECTOR_SIZE) ? d->data[i + 2] : 0;
                uint32_t b3 = (i + 3 < SECTOR_SIZE) ? d->data[i + 3] : 0;
                return (b0) | (b1 << 8) | (b2 << 16) | (b3 << 24);
            }
            return 0;
    }
}

static void disk_mmio_write(void *ctx, uint32_t addr, uint32_t val) {
    DiskCtl *d = (DiskCtl*)ctx;
    const uint32_t off = addr - d->base;

    switch (off) {
        case REG_LBA:
            d->lba = val;
            break;

        case REG_COUNT:
            d->count = val ? val : 1;   // never 0
            break;

        case REG_CMD: {
            // Log the kick
            log_printf("[DISK] CMD write val=0x%X LBA=%u COUNT=%u STATUS(before)=0x%X\n",
                       val, d->lba, d->count, d->status);

            // Only operation we handle: READ | GO
            if ((val & (CMD_READ | CMD_GO)) == (CMD_READ | CMD_GO)) {
                if (!disk_present(d->slot)) {
                    d->status = ST_ERR;
                    log_printf("[DISK] READ aborted: no image present\n");
                    break;
                }

                d->status = ST_BUSY;

                // Read exactly 1 sector into our exposed buffer (simple PIO model)
                bool ok = disk_read_sectors(d->slot, d->lba, d->data, 1);
                if (!ok) {
                    d->status = ST_ERR;
                    log_printf("[DISK] READ failed: LBA=%u\n", d->lba);
                    break;
                }

                // Make data available and signal DRQ (not BUSY)
                d->status = ST_DRQ;

                // Nice debug: first 16 bytes
                log_printf("[DISK] READ LBA=%u (count=%u) \xE2\x86\x92 DRQ, first16="
                           "%02x %02x %02x %02x %02x %02x %02x %02x "
                           "%02x %02x %02x %02x %02x %02x %02x %02x\n",
                           d->lba, d->count,
                           d->data[0], d->data[1], d->data[2], d->data[3],
                           d->data[4], d->data[5], d->data[6], d->data[7],
                           d->data[8], d->data[9], d->data[10], d->data[11],
                           d->data[12], d->data[13], d->data[14], d->data[15]);

                log_printf("[DISK] CMD handled: STATUS(after)=0x%X\n", d->status);
            } else {
                log_printf("[DISK] unknown CMD=0x%X\n", val);
            }
            break;
        }

        case REG_STATUS:
            // read-only; ignore writes
            break;

        default:
            // ignore unhandled offsets
            break;
    }
}

bool dev_disk0_present(void) {
    return disk_present(0);
}

bool dev_disk0_attach(VM *vm, const char *image_path) {
    if (!g_disk0.mapped) {
        g_disk0.vm     = vm;
        g_disk0.base   = DISK0_BASE;
        g_disk0.slot   = 0;
        g_disk0.lba    = 0;
        g_disk0.count  = 1;
        g_disk0.status = 0;
        memset(g_disk0.data, 0, sizeof(g_disk0.data));

        // Map at least up to DATA+512; 0x1000 is a simple page.
        if (!vm_map_mmio(vm, g_disk0.base, 0x1000, disk_mmio_read, disk_mmio_write, &g_disk0)) {
            log_printf("[ERROR] dev_disk0: vm_map_mmio failed\n");
            return false;
        }
        g_disk0.mapped = true;
    }

    static bool dm_inited = false;
    if (!dm_inited) { disk_init(); dm_inited = true; }

    if (!disk_attach(0, image_path, false)) {
        log_printf("[ERROR] attach disk0 failed: %s\n", image_path);
        return false;
    }

    const size_t bytes = disk_size_bytes(0);
    log_printf("[DISK] disk0 attached at 0x%08X (%zu bytes)\n", g_disk0.base, bytes);
    return true;
}

// Optional debug helpers mirroring earlier versions (ok to keep)
uint32_t dev_disk0_read_reg(uint32_t addr) {
    return disk_mmio_read(&g_disk0, addr);
}

void dev_disk0_write_reg(uint32_t addr, uint32_t val) {
    disk_mmio_write(&g_disk0, addr, val);
}
