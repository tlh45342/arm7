#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Init / pump / present
void wincrt_init_text(int cols, int rows);
void wincrt_pump_messages(void);
void wincrt_present_text(const uint8_t *vram, int x, int y, int cols, int rows);

// High-level helpers used by the VM
void show_crt(void);
void crt_refresh(void);
void crt_mark_dirty(uint32_t addr, size_t len);

#ifdef __cplusplus
} // extern "C"
#endif
