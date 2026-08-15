#pragma once
#include <stdint.h>
#include <stdbool.h>
#define KBD_BASE_ADDR 0x0A001000u
#define KBD_MMIO_SIZE 0x00000100u
#define KBD_REG_STATUS 0x00u
#define KBD_REG_DATA 0x04u
#define KBD_STATUS_READY 0x00000001u
void dev_keyboard_init(uint32_t base_addr);
void dev_keyboard_reset(void);
bool dev_keyboard_push(uint8_t ch);
uint32_t dev_keyboard_read32(uint32_t addr);
void dev_keyboard_write32(uint32_t addr,uint32_t value);
unsigned dev_keyboard_count(void);
