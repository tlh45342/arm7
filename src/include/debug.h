// src/include/debug.h
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Bitmask type for debug flags
typedef uint32_t debug_flags_t;

// Common flags (extend as you like)
enum {
    DBG_NONE      = 0,
    DBG_INSTR     = 1u << 0,  // instruction trace
    DBG_MEM_READ  = 1u << 1,
    DBG_MEM_WRITE = 1u << 2,
    DBG_MMIO      = 1u << 3,
    DBG_DISK      = 1u << 4,
    DBG_IRQ       = 1u << 5,
    DBG_DISASM    = 1u << 6,
	DBG_K12       = 1u << 7,
	DBG_TRACE     = 1u << 8,
	DBG_CLI       = 1u << 9,
};

#define DBG_ALL (DBG_INSTR|DBG_MEM_READ|DBG_MEM_WRITE|DBG_MMIO|DBG_DISK|DBG_IRQ|DBG_DISASM|DBG_K12|DBG_TRACE|DBG_CLI)

// These are defined in some .c (e.g., cpu.c for debug_flags, or elsewhere)
extern debug_flags_t debug_flags;
extern bool          trace_all;

static inline bool dbg_enabled_mask(debug_flags_t mask) {
    extern debug_flags_t debug_flags;
    extern bool          trace_all;
    return trace_all || ((debug_flags & mask) != 0);
}

// Generic masked logger
#ifndef DBG_IF
#define DBG_IF(mask, fmt, ...)                                                     \
    do {                                                                           \
        if (dbg_enabled_mask((mask))) {                                            \
            fprintf(stderr, "[dbg] " fmt "\n", ##__VA_ARGS__);                     \
        }                                                                          \
    } while (0)
#endif

// Default logger (uses DBG_TRACE bit)
#ifndef DBG
#define DBG(fmt, ...)        DBG_IF(DBG_TRACE, fmt, ##__VA_ARGS__)
#endif

// Convenience loggers by domain (optional)
#ifndef DBG_INSTRF
#define DBG_INSTRF(fmt, ...) DBG_IF(DBG_INSTR, fmt, ##__VA_ARGS__)
#define DBG_MEMR(fmt, ...)   DBG_IF(DBG_MEM_READ,  fmt, ##__VA_ARGS__)
#define DBG_MEMW(fmt, ...)   DBG_IF(DBG_MEM_WRITE, fmt, ##__VA_ARGS__)
#define DBG_MMIOF(fmt, ...)  DBG_IF(DBG_MMIO, fmt, ##__VA_ARGS__)
#define DBG_K12F(fmt, ...)   DBG_IF(DBG_K12,  fmt, ##__VA_ARGS__)
#define DBG_CLIF(fmt, ...)   DBG_IF(DBG_CLI,  fmt, ##__VA_ARGS__)
#endif

static inline void DEBUGMAX(void) {
    extern debug_flags_t debug_flags;
    extern bool          trace_all;
    trace_all  = true;                 // make DBG_IF(...) print regardless of mask
    debug_flags = (debug_flags_t)~0u;  // also set every bit for code that checks flags directly
}