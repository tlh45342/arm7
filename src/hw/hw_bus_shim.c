// src/hw/hw_bus_shim.c
#include <stdint.h>

// Strong, no-op fallback so the build links even if the real bus
// layer doesn't provide hw_bus_map_region yet.
// If you later add a real implementation with the same symbol,
// just delete this file to avoid duplicate definitions.
void hw_bus_map_region(const char* name,
                       uint32_t base,
                       uint32_t size,
                       uint32_t (*read32)(uint32_t),
                       void (*write32)(uint32_t, uint32_t))
{
    (void)name;
    (void)base;
    (void)size;
    (void)read32;
    (void)write32;
    // Intentionally no-op.
}