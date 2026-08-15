// src/hw/dev_uart.c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "dev_uart.h"

static uint32_t g_uart_base = 0;
static bool     g_uart_ok   = false;

void dev_uart_init(uint32_t base) {
    g_uart_base = base;
    g_uart_ok   = true;
}

bool dev_uart_present(void) {
    return g_uart_ok;
}

uint32_t dev_uart_read_reg(uint32_t addr) {
    if (!g_uart_ok) return 0;
    uint32_t off = addr - g_uart_base;

    switch (off) {
        case UART_FR:
            // TX not full (0), RX empty (RXFE set): lets guest write freely
            return UART_FR_RXFE;
        default:
            // DR reads (RX) and other regs not implemented
            return 0;
    }
}

void dev_uart_write_reg(uint32_t addr, uint32_t val) {
    if (!g_uart_ok) return;
    uint32_t off = addr - g_uart_base;

    switch (off) {
        case UART_DR: {
            uint8_t ch = (uint8_t)val;
            putchar(ch);
            fflush(stdout);
            break;
        }
        default:
            // Ignore unimplemented registers
            break;
    }
}
