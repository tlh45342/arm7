// putchars_uart_c.c — PL011-ish UART "Hello World" (bare-min)
// PC starts at 0x8000; no bss/data init; sets SP; prints; halts with 0xDEADBEEF.

#include <stdint.h>

#define UART_BASE   0x09000000u
#define UART_DR     (UART_BASE + 0x00)   // Data register (TX/RX)
#define UART_FR     (UART_BASE + 0x18)   // Flag register
#define UART_FR_TXFF (1u << 5)           // TX FIFO full

static inline void mmio_w32(uint32_t addr, uint32_t v){ *(volatile uint32_t*)addr = v; }
static inline uint32_t mmio_r32(uint32_t addr){ return *(volatile uint32_t*)addr; }

static void uart_putc(char c) {
    // translate '\n' -> "\r\n"
    if (c == '\n') {
        while (mmio_r32(UART_FR) & UART_FR_TXFF) { /* spin while full */ }
        mmio_w32(UART_DR, (uint32_t)'\r');  // 32-bit write, low byte used
    }
    while (mmio_r32(UART_FR) & UART_FR_TXFF) { /* spin while full */ }
    mmio_w32(UART_DR, (uint32_t)(uint8_t)c);
}

static void uart_write(const char *s){ while (*s) uart_putc(*s++); }

// Naked entry: set SP and call main; never return.
__attribute__((naked, noreturn))
void _start(void) {
    __asm__ volatile (
        "ldr sp, =0x20000000 \n"   // choose a safe stack top for your VM
        "bl  main             \n"
        "b   .                \n"
    );
}

int main(void) {
    uart_write("Hello World.\n");
    __asm__ volatile (".word 0xDEADBEEF");  // your VM treats as HALT
    for(;;) {}
}
