SECTIONS
{
    . = 0x8000;
    .text : { *(.text) }
    .rodata : { *(.rodata) }
}
ENTRY(_start)