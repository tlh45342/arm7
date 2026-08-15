arm-none-eabi-nm test_ldrb.elf

O:\arm-vm\test-cases\test_ldrb>arm-none-eabi-nm test_ldrb.elf
00008000 T _start
00008010 d string

arm-none-eabi-objdump test_-d ldrb.elf

O:\arm-vm\test-cases\test_ldrb>arm-none-eabi-objdump -d test_ldrb.elf

test_ldrb.elf:     file format elf32-littlearm


Disassembly of section .text:

00008000 <_start>:
    8000:       e59f0004        ldr     r0, [pc, #4]    ; 800c <_start+0xc>
    8004:       e4d01001        ldrb    r1, [r0], #1
    8008:       deadbeef        .word   0xdeadbeef
    800c:       00008010        .word   0x00008010