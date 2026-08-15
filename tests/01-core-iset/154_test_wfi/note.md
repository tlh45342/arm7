O:\test-cases\wfi_test>arm-none-eabi-nm wfi_test.elf
00008000 T _start

O:\test-cases\wfi_test>arm-none-eabi-objdump -d wfi_test.elf

wfi_test.elf:     file format elf32-littlearm


Disassembly of section .text:

00008000 <_start>:
    8000:       e320f003        wfi

O:\test-cases\wfi_test>hexdump wfi_test.bin
0000000 f003 e320
0000004