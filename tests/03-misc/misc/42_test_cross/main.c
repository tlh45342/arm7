#include <stdint.h>

void main(void) {
    volatile uint8_t *crt = (volatile uint8_t *)0x0A000000;
    crt[0] = 'A';
    crt[1] = 0x07;
}