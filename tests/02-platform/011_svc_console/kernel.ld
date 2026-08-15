ENTRY(_start)
SECTIONS {
  . = 0x00000000;
  .vectors : { *(.vectors*) }
  .text : { *(.text*) *(.rodata*) }
  .data : { *(.data*) }
  .bss : { *(.bss*) *(COMMON) }
}
