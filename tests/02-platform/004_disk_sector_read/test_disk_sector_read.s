.syntax unified
.arch armv7-a
.arm
.text
.align 2
.global _start

.equ DISK_BASE,   0x0B000000
.equ DISK_LBA,    0x04
.equ DISK_COUNT,  0x08
.equ DISK_CMD,    0x0C
.equ DISK_STATUS, 0x10
.equ DISK_DATA,   0x200

.equ CMD_READ_GO, 0x81
.equ STATUS_DRQ,  0x02
.equ STATUS_ERR,  0x04

.equ DEST,        0x00030000
.equ RESULT,      0x00030200

/*
 * This deliberately mirrors the BIOS sector-read contract:
 *   LBA=0, COUNT=1, CMD=READ|GO, wait for DRQ, copy 128 words.
 *
 * RESULT layout:
 *   +0x00 = 0x44524551 ("QERD") once request is issued
 *   +0x04 = final disk status
 *   +0x08 = first word copied from sector
 *   +0x0C = 0x50415353 ("SSAP") on success
 *   +0x10 = 0x45525221 ("!RRE") on error
 */
_start:
    movw    sp, #0xFFFC
    movt    sp, #0x001F

    movw    r4, #:lower16:DISK_BASE
    movt    r4, #:upper16:DISK_BASE

    movw    r5, #:lower16:DEST
    movt    r5, #:upper16:DEST

    movw    r6, #:lower16:RESULT
    movt    r6, #:upper16:RESULT

    /* LBA 0 */
    mov     r0, #0
    str     r0, [r4, #DISK_LBA]

    /* one sector */
    mov     r0, #1
    str     r0, [r4, #DISK_COUNT]

    /* mark that guest reached the command */
    movw    r0, #0x4551
    movt    r0, #0x4452
    str     r0, [r6, #0x00]

    /* READ | GO */
    mov     r0, #CMD_READ_GO
    str     r0, [r4, #DISK_CMD]

wait_status:
    ldr     r1, [r4, #DISK_STATUS]
    tst     r1, #STATUS_ERR
    bne     disk_error
    tst     r1, #STATUS_DRQ
    beq     wait_status

    /* Copy the full 512-byte DATA window exactly as BIOS needs to. */
    add     r2, r4, #DISK_DATA
    mov     r3, #128
copy_sector:
    ldr     r0, [r2], #4
    str     r0, [r5], #4
    subs    r3, r3, #1
    bne     copy_sector

    str     r1, [r6, #0x04]

    movw    r5, #:lower16:DEST
    movt    r5, #:upper16:DEST
    ldr     r0, [r5]
    str     r0, [r6, #0x08]

    movw    r0, #0x5353
    movt    r0, #0x5041
    str     r0, [r6, #0x0C]

    bkpt    0x1234

disk_error:
    str     r1, [r6, #0x04]
    movw    r0, #0x5221
    movt    r0, #0x4552
    str     r0, [r6, #0x10]
    bkpt    0x1234
