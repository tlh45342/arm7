// src/include/dev_crt.h

#ifndef DEV_CRT_H
#define DEV_CRT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>   // <-- needed for size_t

// Initialize the text CRT device (cols/rows + MMIO base for your layout)
bool dev_crt_init(int cols, int rows, uint32_t mmio_text_base);

// Enable/disable on-screen window (WinCRT) rendering
void dev_crt_set_enabled(bool enabled);
bool dev_crt_is_enabled(void);

// Pump window messages and refresh ~60Hz if dirty or time elapsed
void dev_crt_pump_60hz(void);

// Force an immediate refresh (copies from guest VRAM via mem_read8)
void dev_crt_present_now(void);

// Dump current screen to stdout (ASCII, non-printables as '.')
void dev_crt_dump_text(void);

// Optional: mark dirty if a write overlaps CRT range
void dev_crt_mark_dirty(uint32_t addr, size_t len);

// Shutdown / cleanup (no-op if unused)
void dev_crt_shutdown(void);

#endif // DEV_CRT_H