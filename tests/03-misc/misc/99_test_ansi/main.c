// ansi.c (drop-in)
#include <stdint.h>

#define UART_BASE 0x09000000u
#define UART_DR   (*(volatile uint32_t *)(UART_BASE + 0x00))
#define UART_FR   (*(volatile uint32_t *)(UART_BASE + 0x18))
#define UART_TXFF (1u << 5)  // Transmit FIFO full

static inline void uart_putc(char c) {
    // Wait while TX FIFO is full (your current UART returns FR=0, but this is correct anyway)
    while (UART_FR & UART_TXFF) { /* spin */ }
    UART_DR = (uint32_t)(uint8_t)c;
}

static void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

void main(void) {                  // your start.s should `bl main`
    uart_puts("\x1b[2J\x1b[H");    // clear screen + home
    uart_puts("\x1b[32m");         // set green
    uart_puts("\x1b[5;10HHello");  // move to row 5, col 10 and print
    uart_puts("\x1b[0m");          // reset attributes
    uart_puts("\r\n");             // newline so the prompt doesn't sit on the line

    for (;;){ /* idle */ }
}