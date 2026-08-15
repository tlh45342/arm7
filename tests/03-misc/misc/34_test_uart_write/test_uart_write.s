  .syntax unified
    .cpu cortex-a15
    .arch armv7-a
    .text
    .global _start

    .equ UART_BASE,  0x09000000
    .equ UART_DR,    0x00
    .equ UART_FR,    0x18
    .equ TXFF_MASK,  0x20

_start:
    ldr   r0, =msg
    bl    puts
    ldr   r0, =done_msg
    bl    puts

    .word  0xE320F000      // NOP
    .word  0xDEADBEEF      // halt marker for your VM

// void uart_putc(uint32_t ch in r0) — poll FR.TXFF
uart_putc:
1:  ldr   r1, =UART_BASE
    ldr   r2, [r1, #UART_FR]
    and   r3, r2, #TXFF_MASK   // r3 = FR & TXFF
    cmp   r3, #0               // TXFF==0 ? ready : busy
    bne   1b
    strb  r0, [r1, #UART_DR]   // write byte
    bx    lr

// void puts(const char* s in r0)
puts:
    push  {r4,lr}
    mov   r4, r0
0:  ldrb  r0, [r4], #1
    cmp   r0, #0
    beq   2f
    bl    uart_putc
    b     0b
2:  pop   {r4,lr}
    bx    lr

    .align 4
msg:       .asciz "Hello, UART!\\n"
done_msg:  .asciz "[test_uart_write] done\\n"
