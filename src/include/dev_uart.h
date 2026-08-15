// src/include/dev_uart.h
#pragma once
#include <stdbool.h>
#include <stdint.h>

// PL011-ish offsets used by the tests
#define UART_DR        0x00u
#define UART_FR        0x18u
#define UART_FR_TXFF   (1u << 5)  // TX FIFO full
#define UART_FR_RXFE   (1u << 4)  // RX FIFO empty

// Init once at startup with the MMIO base (tests use 0x0900_0000)
void     dev_uart_init(uint32_t base);

// Present flag for mem.c dispatch
bool     dev_uart_present(void);

// 32-bit register access used by mem.c (it may RMW for byte writes)
uint32_t dev_uart_read_reg(uint32_t addr);
void     dev_uart_write_reg(uint32_t addr, uint32_t val);
