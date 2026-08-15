// include/hw.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct HWDevice HWDevice;

typedef uint32_t (*hw_read32_fn)(HWDevice*, uint32_t offset);
typedef void     (*hw_write32_fn)(HWDevice*, uint32_t offset, uint32_t val);
typedef void     (*hw_tick_fn)(HWDevice*, int budget);
typedef void     (*hw_reset_fn)(HWDevice*);
typedef const char* (*hw_name_fn)(HWDevice*);

struct HWDevice {
    uint32_t base, size;
    hw_read32_fn  read32;
    hw_write32_fn write32;
    hw_tick_fn    tick;     // may be NULL if not time-based
    hw_reset_fn   reset;    // optional
    hw_name_fn    name;     // optional
    void*         impl;     // device-private state
};

// Bus API
void hw_bus_init(void);
bool hw_bus_attach(HWDevice* dev);             // returns false on overlap
bool hw_bus_read32(uint32_t addr, uint32_t* out);
bool hw_bus_write32(uint32_t addr, uint32_t val);
void hw_bus_tick(int cycles_budget);           // calls all device ticks
void hw_bus_reset(void);
