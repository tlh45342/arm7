#include "hw.h"
#include <stdio.h>

#define MAX_DEVS 32

static HWDevice* g_devs[MAX_DEVS];
static int       g_ndevs = 0;

// simple per-class scaling (expand as you add devices)
static int g_disk_bpc = 64;   // disk bytes per *CPU cycle* (default)
static int g_uart_bpc = 1;    // uart bytes per cycle
static int g_crt_bpc  = 0;    // no default work

void hw_bus_set_scale(int disk_bpc, int uart_bpc, int crt_bpc) {
    if (disk_bpc >= 0) g_disk_bpc = disk_bpc;
    if (uart_bpc >= 0) g_uart_bpc = uart_bpc;
    if (crt_bpc  >= 0) g_crt_bpc  = crt_bpc;
}

void hw_bus_init(void) {
    g_ndevs = 0;
}

static bool overlaps(uint32_t a0, uint32_t a1, uint32_t b0, uint32_t b1) {
    return !(b1 <= a0 || a1 <= b0);
}

bool hw_bus_attach(HWDevice* d) {
    if (g_ndevs >= MAX_DEVS) return false;
    for (int i = 0; i < g_ndevs; i++) {
        uint32_t a0 = g_devs[i]->base, a1 = a0 + g_devs[i]->size;
        uint32_t b0 = d->base,         b1 = b0 + d->size;
        if (overlaps(a0, a1, b0, b1)) return false;
    }
    g_devs[g_ndevs++] = d;
    return true;
}

static HWDevice* find(uint32_t addr, uint32_t* off_out) {
    for (int i = 0; i < g_ndevs; i++) {
        HWDevice* d = g_devs[i];
        if (addr - d->base < d->size) {
            *off_out = addr - d->base;
            return d;
        }
    }
    return NULL;
}

bool hw_bus_read32(uint32_t addr, uint32_t* out) {
    uint32_t off; HWDevice* d = find(addr, &off);
    if (!d || !d->read32) return false;
    *out = d->read32(d, off & ~3u);
    return true;
}

bool hw_bus_write32(uint32_t addr, uint32_t val) {
    uint32_t off; HWDevice* d = find(addr, &off);
    if (!d || !d->write32) return false;
    d->write32(d, off & ~3u, val);
    return true;
}

/**
 * Drive all devices once per CPU instruction.
 * We translate CPU cycles to device “byte budgets”.
 * You can refine this: per-device type tags, hz ratios, etc.
 */
void hw_bus_tick(int cycles) {
    if (cycles <= 0) cycles = 1;

    for (int i = 0; i < g_ndevs; i++) {
        HWDevice* d = g_devs[i];
        if (!d->tick) continue;

        // crude classification based on name() prefix; you can instead
        // add a 'kind' enum into HWDevice if you prefer.
        int budget = 0;
        const char* nm = d->name ? d->name(d) : "";

        if (nm && nm[0]) {
            if      (nm[0]=='d' && nm[1]=='i' && nm[2]=='s' && nm[3]=='k') budget = g_disk_bpc * cycles;
            else if (nm[0]=='u' && nm[1]=='a' && nm[2]=='r' && nm[3]=='t') budget = g_uart_bpc * cycles;
            else if (nm[0]=='c' && nm[1]=='r' && nm[2]=='t')               budget = g_crt_bpc  * cycles;
            else                                                            budget = 0; // unknown device kind
        } else {
            budget = 0;
        }

        if (budget > 0) d->tick(d, budget);
    }
}
