.syntax unified
.arch armv7-a
.arm

.equ CRT_BASE,      0x0A000000
.equ CRT_COLS,      80
.equ CRT_ROWS,      25
.equ CRT_CELLS,     (CRT_COLS * CRT_ROWS)
.equ CRT_CELL_SIZE, 2
.equ CRT_ATTR,      0x07

.equ STACK_TOP,     0x001FF000
.equ BOOT_ADDR,     0x00010000
.equ HEADER_BUF,    0x00030000
.equ DIR_BUF,       0x00030400

.equ DISK_BASE,     0x0B000000
.equ DISK_LBA,      0x04
.equ DISK_COUNT,    0x08
.equ DISK_CMD,      0x0C
.equ DISK_STATUS,   0x10
.equ DISK_DATA,     0x200
.equ CMD_READ_GO,   0x81
.equ STATUS_DRQ,    0x02
.equ STATUS_ERR,    0x04

.equ SFLT_SECTOR_SIZE_OFF, 8
.equ SFLT_DIR_LBA_OFF,     16
.equ SFLT_DIR_ENTRIES_OFF, 20
.equ SFLT_DIRENT_SIZE_OFF, 24
.equ SFLT_DIRENT_SIZE,     64
.equ SFLT_DIRENTS_PER_SEC, 8

.section .vectors, "ax"
.align 5
.global _start
_start:
    b reset
    b vector_undef
    b vector_svc
    b vector_pabort
    b vector_dabort
    b vector_reserved
    b vector_irq
    b vector_fiq

.section .text, "ax"
.align 2
reset:
    movw sp, #:lower16:STACK_TOP
    movt sp, #:upper16:STACK_TOP

    movw r4, #:lower16:CRT_BASE
    movt r4, #:upper16:CRT_BASE

    /*
     * Keep early startup deliberately stack-free.  The previous v0.0.3
     * wandered into CRT/MMIO before the first disk command.  These loops
     * do not BL and do not PUSH/POP.
     */
    mov r0, #0x20
    mov r3, #CRT_ATTR
    movw r1, #CRT_CELLS
    mov r2, r4
1:
    strb r0, [r2]
    strb r3, [r2, #1]
    add r2, r2, #2
    subs r1, r1, #1
    bne 1b

    /* Inline the banner too: no call/return before disk activity. */
    movw r0, #:lower16:bios_banner
    movt r0, #:upper16:bios_banner
    mov r1, r4
    mov r3, #CRT_ATTR
2:
    ldrb r2, [r0], #1
    cmp r2, #0
    beq 3f
    strb r2, [r1]
    strb r3, [r1, #1]
    add r1, r1, #2
    b 2b
3:
    /* Read SIMPLE-FLAT header, using a dedicated link register r12. */
    mov r0, #0
    movw r1, #:lower16:HEADER_BUF
    movt r1, #:upper16:HEADER_BUF
    adr r12, after_header_read
    b disk_read_sector
after_header_read:
    cmp r0, #0
    beq boot_disk_error

    movw r6, #:lower16:HEADER_BUF
    movt r6, #:upper16:HEADER_BUF

    ldr r0, [r6]
    movw r1, #0x4653
    movt r1, #0x544C
    cmp r0, r1
    bne boot_format_error

    ldr r0, [r6, #SFLT_SECTOR_SIZE_OFF]
    cmp r0, #512
    bne boot_format_error

    ldr r8, [r6, #SFLT_DIR_LBA_OFF]
    ldr r9, [r6, #SFLT_DIR_ENTRIES_OFF]
    ldr r10, [r6, #SFLT_DIRENT_SIZE_OFF]
    cmp r8, #0
    beq boot_format_error
    cmp r9, #0
    beq boot_not_found
    cmp r10, #SFLT_DIRENT_SIZE
    bne boot_format_error

directory_sector_loop:
    mov r0, r8
    movw r1, #:lower16:DIR_BUF
    movt r1, #:upper16:DIR_BUF
    adr r12, after_directory_read
    b disk_read_sector
after_directory_read:
    cmp r0, #0
    beq boot_disk_error

    movw r6, #:lower16:DIR_BUF
    movt r6, #:upper16:DIR_BUF
    mov r7, #SFLT_DIRENTS_PER_SEC

directory_entry_loop:
    cmp r9, #0
    beq boot_not_found

    ldr r0, [r6]
    movw r1, #0x4F42
    movt r1, #0x544F
    cmp r0, r1
    bne next_directory_entry

    ldr r0, [r6, #4]
    movw r1, #0x422E
    movt r1, #0x4E49
    cmp r0, r1
    bne next_directory_entry

    ldrb r0, [r6, #8]
    cmp r0, #0
    beq boot_entry_found

next_directory_entry:
    add r6, r6, #SFLT_DIRENT_SIZE
    subs r9, r9, #1
    beq boot_not_found
    subs r7, r7, #1
    bne directory_entry_loop
    add r8, r8, #1
    b directory_sector_loop

boot_entry_found:
    ldr r8, [r6, #32]
    ldr r9, [r6, #36]
    cmp r9, #0
    beq boot_not_found

    add r9, r9, #0x100
    add r9, r9, #0x0FF
    lsr r9, r9, #9

    movw r10, #:lower16:BOOT_ADDR
    movt r10, #:upper16:BOOT_ADDR

load_boot_loop:
    mov r0, r8
    mov r1, r10
    adr r12, after_boot_sector_read
    b disk_read_sector
after_boot_sector_read:
    cmp r0, #0
    beq boot_disk_error
    add r8, r8, #1
    add r10, r10, #512
    subs r9, r9, #1
    bne load_boot_loop

    /* Inline success message; still no stack dependency. */
    movw r0, #:lower16:boot_loaded_msg
    movt r0, #:upper16:boot_loaded_msg
    mov r1, r4
    add r1, r1, #(CRT_COLS * CRT_CELL_SIZE)
    mov r3, #CRT_ATTR
4:
    ldrb r2, [r0], #1
    cmp r2, #0
    beq 5f
    strb r2, [r1]
    strb r3, [r1, #1]
    add r1, r1, #2
    b 4b
5:
    movw r0, #:lower16:BOOT_ADDR
    movt r0, #:upper16:BOOT_ADDR
    bx r0

/*
 * Stack-free sector primitive.
 * Entry: r0=LBA, r1=destination, r12=continuation address.
 * Return: r0=1/0, branch through r12.
 */
disk_read_sector:
    movw r2, #:lower16:DISK_BASE
    movt r2, #:upper16:DISK_BASE
    str r0, [r2, #DISK_LBA]
    mov r3, #1
    str r3, [r2, #DISK_COUNT]
    mov r3, #CMD_READ_GO
    str r3, [r2, #DISK_CMD]
6:
    ldr r3, [r2, #DISK_STATUS]
    tst r3, #STATUS_ERR
    bne 8f
    tst r3, #STATUS_DRQ
    beq 6b
    add r2, r2, #DISK_DATA
    mov r3, #128
7:
    ldr r5, [r2], #4
    str r5, [r1], #4
    subs r3, r3, #1
    bne 7b
    mov r0, #1
    bx r12
8:
    mov r0, #0
    bx r12

boot_disk_error:
    movw r0, #:lower16:disk_error_msg
    movt r0, #:upper16:disk_error_msg
    b show_error
boot_format_error:
    movw r0, #:lower16:format_error_msg
    movt r0, #:upper16:format_error_msg
    b show_error
boot_not_found:
    movw r0, #:lower16:not_found_msg
    movt r0, #:upper16:not_found_msg
show_error:
    mov r1, r4
    add r1, r1, #(CRT_COLS * CRT_CELL_SIZE)
    mov r3, #CRT_ATTR
9:
    ldrb r2, [r0], #1
    cmp r2, #0
    beq 10f
    strb r2, [r1]
    strb r3, [r1, #1]
    add r1, r1, #2
    b 9b
10:
    bkpt #0x0200

vector_undef:     bkpt #0x0101
vector_svc:       bkpt #0x0102
vector_pabort:    bkpt #0x0103
vector_dabort:    bkpt #0x0104
vector_reserved:  bkpt #0x0105
vector_irq:       bkpt #0x0106
vector_fiq:       bkpt #0x0107

.align 2
bios_banner:       .asciz "ARM7 BIOS v0.0.3"
boot_loaded_msg:   .asciz "SIMPLE-FLAT: BOOT.BIN loaded"
disk_error_msg:    .asciz "BIOS ERROR: disk0 read failed"
format_error_msg:  .asciz "BIOS ERROR: not SIMPLE-FLAT"
not_found_msg:     .asciz "BIOS ERROR: BOOT.BIN not found"
