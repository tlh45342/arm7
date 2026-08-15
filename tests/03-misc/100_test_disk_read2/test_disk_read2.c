// test_disk_read2.c — read LBA 0 and print it over UART, bare metal, single C file.

#include <stdint.h>

/* ---------- UART: PL011-ish (from dev_uart.h) ---------- */
#define UART_BASE     0x09000000u
#define UART_DR       (UART_BASE + 0x00)
#define UART_FR       (UART_BASE + 0x18)
#define UART_FR_TXFF  (1u << 5)   // TX FIFO full

/* ---------- DISK MMIO (your earlier layout) ---------- */
#define DISK_BASE     0x0B000000u
#define DISK_DATA     (DISK_BASE + 0x200)  // 512B window
#define DISK_LBA      (DISK_BASE + 0x04)
#define DISK_COUNT    (DISK_BASE + 0x08)
#define DISK_CMD      (DISK_BASE + 0x0C)   // bit7=GO, low7=opcode
#define DISK_STATUS   (DISK_BASE + 0x10)   // bit0=BUSY, bit1=DRQ, bit2=ERR

#define CMD_READ      0x01
#define CMD_GO        0x80
#define ST_BUSY       0x01
#define ST_DRQ        0x02
#define ST_ERR        0x04

/* ---------- MMIO helpers ---------- */
static inline void     mmio_w32(uint32_t a, uint32_t v){ *(volatile uint32_t*)a = v; }
static inline uint32_t mmio_r32(uint32_t a){ return *(volatile uint32_t*)a; }

/* ---------- UART ---------- */
static void uart_putc(char c){
    if (c=='\n'){ while (mmio_r32(UART_FR) & UART_FR_TXFF){} mmio_w32(UART_DR, '\r'); }
    while (mmio_r32(UART_FR) & UART_FR_TXFF){}
    mmio_w32(UART_DR, (uint32_t)(uint8_t)c);
}
static void uart_write(const char *s){ while (*s) uart_putc(*s++); }

static void uart_hex8(uint8_t b){
    const char *hex = "0123456789ABCDEF";
    uart_putc(hex[(b>>4)&0xF]); uart_putc(hex[b&0xF]);
}
static void uart_hex32(uint32_t v){
    for (int i=7;i>=0;--i) uart_putc("0123456789ABCDEF"[(v>>(i*4))&0xF]);
}

/* ---------- DISK: read one 512B sector into dst ---------- */
static int disk_read_sector(uint32_t lba, uint8_t *dst){
    // program request
    mmio_w32(DISK_LBA,   lba);
    mmio_w32(DISK_COUNT, 1);
    mmio_w32(DISK_CMD,   CMD_GO | CMD_READ);

    // wait for BUSY to drop (optional: also break on ERR)
    while (mmio_r32(DISK_STATUS) & ST_BUSY){
        if (mmio_r32(DISK_STATUS) & ST_ERR) return -1;
    }

    // wait for DRQ
    while ((mmio_r32(DISK_STATUS) & ST_DRQ)==0){
        if (mmio_r32(DISK_STATUS) & ST_ERR) return -2;
    }

    // read 512 bytes; window is 32-bit readable, so do words
    for (int i=0;i<512;i+=4){
        uint32_t w = mmio_r32(DISK_DATA + (uint32_t)i);
        dst[i+0] = (uint8_t)(w);
        dst[i+1] = (uint8_t)(w>>8);
        dst[i+2] = (uint8_t)(w>>16);
        dst[i+3] = (uint8_t)(w>>24);
    }

    // final status check
    if (mmio_r32(DISK_STATUS) & ST_ERR) return -3;
    return 0;
}

/* ---------- simple hex+ASCII dump ---------- */
static void dump512(const uint8_t *p){
    for (int off=0; off<512; off+=16){
        uart_hex32((uint32_t)off); uart_write(": ");
        for (int i=0;i<16;i++){ uart_hex8(p[off+i]); uart_putc(i==7?' ':' '); }
        uart_write(" |");
        for (int i=0;i<16;i++){
            char c = (char)p[off+i];
            uart_putc((c>=32 && c<127) ? c : '.');
        }
        uart_write("|\n");
    }
}

/* ---------- Entry (no CRT) ---------- */
__attribute__((naked, noreturn))
void _start(void){
    __asm__ volatile(
        "ldr sp, =0x20000000 \n"   // pick a safe top-of-stack
        "bl  main             \n"
        "b   .                \n"
    );
}

int main(void){
    uart_write("disk_read2: reading LBA 0...\n");
    uint8_t buf[512];  // on stack (avoid .bss)
    int rc = disk_read_sector(0, buf);
    if (rc==0){
        uart_write("OK. Dumping 512 bytes:\n");
        dump512(buf);
    } else {
        uart_write("READ ERROR rc="); uart_hex32((uint32_t)rc); uart_putc('\n');
    }

    __asm__ volatile(".word 0xDEADBEEF"); // clean halt in your VM
    for(;;){}
}
