ENTRY(_start)
SECTIONS
{
    . = 0x00020000;
    .text : { *(.text*) *(.rodata*) }
    .data : { *(.data*) }
    .bss  : { *(.bss*) *(COMMON) }
}
