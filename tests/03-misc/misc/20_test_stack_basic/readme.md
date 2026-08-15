O:\test-cases\test0>arm-none-eabi-nm test_stack.elf
00008000 T _start
00008014 t hang

O:\test-cases\test0>arm-none-eabi-objdump -d test_stack.elf

test_stack.elf:     file format elf32-littlearm


Disassembly of section .text:

00008000 <_start>:
    8000:       e3a01011        mov     r1, #17
    8004:       e3a02022        mov     r2, #34 ; 0x22
    8008:       e3a03033        mov     r3, #51 ; 0x33
    800c:       e92d000e        push    {r1, r2, r3}
    8010:       e8bd0070        pop     {r4, r5, r6}

00008014 <hang>:
    8014:       eafffffe        b       8014 <hang>