ENTRY(_start)
SECTIONS { . = 0; .vectors : { *(.vectors*) } .text : { *(.text*) *(.rodata*) } }
