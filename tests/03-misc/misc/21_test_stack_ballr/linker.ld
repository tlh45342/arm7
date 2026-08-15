ENTRY(_start)

SECTIONS {
    . = 0x8000;  /* Set the program's load address */

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
        *(.rodata)
    }

    .bss : {
        *(.bss)
        *(COMMON)
    }
}