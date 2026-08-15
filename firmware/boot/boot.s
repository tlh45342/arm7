.syntax unified
.arch armv7-a
.arm

.equ CRT_BASE,      0x0A000000
.equ CRT_COLS,      80
.equ CRT_ATTR,      0x07

.equ MONITOR_ADDR,  0x00020000
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

.section .text, "ax"
.align 2
.global _start

_start:
    /*
     * BOOT is entered by BIOS at 0x00010000.
     * Keep this path deliberately simple and stack-free, mirroring
     * the proven isolated 008-010 loader tests.
     */

    movw    r4, #:lower16:CRT_BASE
    movt    r4, #:upper16:CRT_BASE

    /* Print BOOT banner on row 1. */
    movw    r0, #:lower16:boot_banner
    movt    r0, #:upper16:boot_banner
    mov     r1, r4
    add     r1, r1, #(CRT_COLS * 2)
    mov     r3, #CRT_ATTR
1:
    ldrb    r2, [r0], #1
    cmp     r2, #0
    beq     2f
    strb    r2, [r1]
    strb    r3, [r1, #1]
    add     r1, r1, #2
    b       1b
2:
    /* Read SIMPLE-FLAT header at LBA 0. */
    mov     r0, #0
    movw    r1, #:lower16:HEADER_BUF
    movt    r1, #:upper16:HEADER_BUF
    adr     r12, after_header_read
    b       disk_read_sector

after_header_read:
    cmp     r0, #0
    beq     boot_disk_error

    movw    r6, #:lower16:HEADER_BUF
    movt    r6, #:upper16:HEADER_BUF

    ldr     r0, [r6]
    movw    r1, #0x4653
    movt    r1, #0x544C          /* "SFLT" */
    cmp     r0, r1
    bne     boot_format_error

    ldr     r0, [r6, #SFLT_SECTOR_SIZE_OFF]
    cmp     r0, #512
    bne     boot_format_error

    ldr     r8, [r6, #SFLT_DIR_LBA_OFF]
    ldr     r9, [r6, #SFLT_DIR_ENTRIES_OFF]
    ldr     r10,[r6, #SFLT_DIRENT_SIZE_OFF]

    cmp     r8, #0
    beq     boot_format_error
    cmp     r9, #0
    beq     monitor_not_found
    cmp     r10, #SFLT_DIRENT_SIZE
    bne     boot_format_error

directory_sector_loop:
    mov     r0, r8
    movw    r1, #:lower16:DIR_BUF
    movt    r1, #:upper16:DIR_BUF
    adr     r12, after_directory_read
    b       disk_read_sector

after_directory_read:
    cmp     r0, #0
    beq     boot_disk_error

    movw    r6, #:lower16:DIR_BUF
    movt    r6, #:upper16:DIR_BUF
    mov     r7, #SFLT_DIRENTS_PER_SEC

directory_entry_loop:
    cmp     r9, #0
    beq     monitor_not_found

    /* MONITOR.BIN */
    ldr     r0, [r6, #0]
    movw    r1, #0x4F4D
    movt    r1, #0x494E          /* MONI */
    cmp     r0, r1
    bne     next_directory_entry

    ldr     r0, [r6, #4]
    movw    r1, #0x4F54
    movt    r1, #0x2E52          /* TOR. */
    cmp     r0, r1
    bne     next_directory_entry

    ldr     r0, [r6, #8]
    movw    r1, #0x4942
    movt    r1, #0x004E          /* BIN\0 */
    cmp     r0, r1
    beq     monitor_entry_found

next_directory_entry:
    add     r6, r6, #SFLT_DIRENT_SIZE
    subs    r9, r9, #1
    beq     monitor_not_found
    subs    r7, r7, #1
    bne     directory_entry_loop

    add     r8, r8, #1
    b       directory_sector_loop

monitor_entry_found:
    ldr     r8, [r6, #32]        /* start LBA */
    ldr     r9, [r6, #36]        /* byte length */

    cmp     r9, #0
    beq     monitor_not_found

    /* ceil(length / 512) */
    add     r9, r9, #0x100
    add     r9, r9, #0x0FF
    lsr     r9, r9, #9

    movw    r10, #:lower16:MONITOR_ADDR
    movt    r10, #:upper16:MONITOR_ADDR

load_monitor_loop:
    mov     r0, r8
    mov     r1, r10
    adr     r12, after_monitor_sector_read
    b       disk_read_sector

after_monitor_sector_read:
    cmp     r0, #0
    beq     boot_disk_error

    add     r8, r8, #1
    add     r10, r10, #512
    subs    r9, r9, #1
    bne     load_monitor_loop

    /* Print a short success note on row 2. */
    movw    r0, #:lower16:monitor_loaded_msg
    movt    r0, #:upper16:monitor_loaded_msg
    mov     r1, r4
    add     r1, r1, #(CRT_COLS * 4)
    mov     r3, #CRT_ATTR
3:
    ldrb    r2, [r0], #1
    cmp     r2, #0
    beq     4f
    strb    r2, [r1]
    strb    r3, [r1, #1]
    add     r1, r1, #2
    b       3b

4:
    movw    r0, #:lower16:MONITOR_ADDR
    movt    r0, #:upper16:MONITOR_ADDR
    bx      r0

/*
 * stack-free disk sector primitive.
 * Entry: r0=LBA, r1=destination, r12=continuation address.
 */
disk_read_sector:
    movw    r2, #:lower16:DISK_BASE
    movt    r2, #:upper16:DISK_BASE

    str     r0, [r2, #DISK_LBA]
    mov     r3, #1
    str     r3, [r2, #DISK_COUNT]
    mov     r3, #CMD_READ_GO
    str     r3, [r2, #DISK_CMD]

5:
    ldr     r3, [r2, #DISK_STATUS]
    tst     r3, #STATUS_ERR
    bne     7f
    tst     r3, #STATUS_DRQ
    beq     5b

    add     r2, r2, #DISK_DATA
    mov     r3, #128
6:
    ldr     r5, [r2], #4
    str     r5, [r1], #4
    subs    r3, r3, #1
    bne     6b

    mov     r0, #1
    bx      r12

7:
    mov     r0, #0
    bx      r12

boot_disk_error:
    movw    r0, #:lower16:disk_error_msg
    movt    r0, #:upper16:disk_error_msg
    b       show_error

boot_format_error:
    movw    r0, #:lower16:format_error_msg
    movt    r0, #:upper16:format_error_msg
    b       show_error

monitor_not_found:
    movw    r0, #:lower16:not_found_msg
    movt    r0, #:upper16:not_found_msg

show_error:
    mov     r1, r4
    add     r1, r1, #(CRT_COLS * 4)
    mov     r3, #CRT_ATTR
8:
    ldrb    r2, [r0], #1
    cmp     r2, #0
    beq     9f
    strb    r2, [r1]
    strb    r3, [r1, #1]
    add     r1, r1, #2
    b       8b
9:
    bkpt    #0x0300

.align 2
boot_banner:
    .asciz "ARM7 BOOT v0.0.2"
monitor_loaded_msg:
    .asciz "MONITOR.BIN loaded"
disk_error_msg:
    .asciz "BOOT ERROR: disk0 read failed"
format_error_msg:
    .asciz "BOOT ERROR: not SIMPLE-FLAT"
not_found_msg:
    .asciz "BOOT ERROR: MONITOR.BIN not found"
