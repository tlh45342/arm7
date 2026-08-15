.syntax unified
.arch armv7-a
.arm
.text
.align 2
.global _start

.equ DISK_BASE,    0x0B000000
.equ DISK_LBA,     0x04
.equ DISK_COUNT,   0x08
.equ DISK_CMD,     0x0C
.equ DISK_STATUS,  0x10
.equ DISK_DATA,    0x200

.equ CMD_READ_GO,  0x81
.equ STATUS_DRQ,   0x02
.equ STATUS_ERR,   0x04

.equ HEADER_BUF,   0x00030000
.equ DIR_BUF,      0x00030400
.equ RESULT,       0x00030800
.equ BOOT_LOAD,    0x00010000

_start:
    movw    sp, #0xFFFC
    movt    sp, #0x001F

    movw    r10, #:lower16:RESULT
    movt    r10, #:upper16:RESULT

    /* Read and validate SIMPLE-FLAT header. */
    mov     r0, #0
    movw    r1, #:lower16:HEADER_BUF
    movt    r1, #:upper16:HEADER_BUF
    bl      disk_read_sector
    cmp     r0, #0
    beq     fail_disk

    movw    r4, #:lower16:HEADER_BUF
    movt    r4, #:upper16:HEADER_BUF

    ldr     r5, [r4]
    movw    r6, #0x4653
    movt    r6, #0x544C
    cmp     r5, r6
    bne     fail_magic

    ldr     r7, [r4, #16]       /* directory LBA */
    ldr     r8, [r4, #20]       /* directory entries */
    ldr     r9, [r4, #24]       /* entry size */
    cmp     r9, #64
    bne     fail_header

    /* Read first directory sector. */
    mov     r0, r7
    movw    r1, #:lower16:DIR_BUF
    movt    r1, #:upper16:DIR_BUF
    bl      disk_read_sector
    cmp     r0, #0
    beq     fail_disk

    /* Scan the 8 entries resident in this sector for BOOT.BIN. */
    movw    r4, #:lower16:DIR_BUF
    movt    r4, #:upper16:DIR_BUF
    mov     r5, #8

scan_entry:
    ldr     r6, [r4]
    movw    r7, #0x4F42
    movt    r7, #0x544F
    cmp     r6, r7
    bne     next_entry

    ldr     r6, [r4, #4]
    movw    r7, #0x422E
    movt    r7, #0x4E49
    cmp     r6, r7
    bne     next_entry

    /* Preserve discovered BOOT metadata. */
    ldr     r6, [r4, #32]       /* start LBA */
    ldr     r7, [r4, #36]       /* byte length */
    ldr     r8, [r4, #40]       /* flags */

    str     r6, [r10, #0]
    str     r7, [r10, #4]
    str     r8, [r10, #8]

    /*
     * sectors = ceil(byte_length / 512)
     * Split +511 into encodable ARM immediates.
     */
    mov     r9, r7
    add     r9, r9, #0x100
    add     r9, r9, #0x0FF
    lsr     r9, r9, #9
    str     r9, [r10, #12]

    /* Load every required sector contiguously at BOOT_LOAD. */
    movw    r11, #:lower16:BOOT_LOAD
    movt    r11, #:upper16:BOOT_LOAD
    mov     r5, r6              /* current LBA */
    mov     r4, r9              /* sectors remaining */

load_loop:
    mov     r0, r5
    mov     r1, r11
    bl      disk_read_sector
    cmp     r0, #0
    beq     fail_disk

    add     r5, r5, #1
    add     r11, r11, #512
    subs    r4, r4, #1
    bne     load_loop

    /* Record first and last payload words for independent guest evidence. */
    movw    r4, #:lower16:BOOT_LOAD
    movt    r4, #:upper16:BOOT_LOAD
    ldr     r0, [r4]
    str     r0, [r10, #16]

    /* last byte address = BOOT_LOAD + byte_length - 1 */
    add     r4, r4, r7
    sub     r4, r4, #1
    ldrb    r0, [r4]
    str     r0, [r10, #20]

    movw    r0, #0x4B4F
    movt    r0, #0x444C          /* bytes "OKLD" */
    str     r0, [r10, #24]

    bkpt    0x1234

next_entry:
    add     r4, r4, #64
    subs    r5, r5, #1
    bne     scan_entry
    b       fail_not_found

/*
 * r0=LBA, r1=destination. Return r0=1 success / 0 failure.
 * Same proven MMIO primitive used by 004 and 005.
 */
disk_read_sector:
    movw    r2, #:lower16:DISK_BASE
    movt    r2, #:upper16:DISK_BASE
    str     r0, [r2, #DISK_LBA]
    mov     r3, #1
    str     r3, [r2, #DISK_COUNT]
    mov     r3, #CMD_READ_GO
    str     r3, [r2, #DISK_CMD]

wait_status:
    ldr     r3, [r2, #DISK_STATUS]
    tst     r3, #STATUS_ERR
    bne     disk_read_fail
    tst     r3, #STATUS_DRQ
    beq     wait_status

    add     r2, r2, #DISK_DATA
    mov     r3, #128
copy_sector:
    ldr     r0, [r2], #4
    str     r0, [r1], #4
    subs    r3, r3, #1
    bne     copy_sector

    mov     r0, #1
    bx      lr

disk_read_fail:
    mov     r0, #0
    bx      lr

fail_disk:
    mov     r0, #1
    b       fail
fail_magic:
    mov     r0, #2
    b       fail
fail_header:
    mov     r0, #3
    b       fail
fail_not_found:
    mov     r0, #4
fail:
    str     r0, [r10, #28]
    bkpt    0x1234
