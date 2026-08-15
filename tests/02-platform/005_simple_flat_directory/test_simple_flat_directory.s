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

/* SIMPLE-FLAT v0.1 inherited layout:
 * header:
 *   +0  magic "SFLT"
 *   +8  sector size
 *   +12 total sectors
 *   +16 directory LBA
 *   +20 directory entries
 *   +24 directory entry size
 *   +28 data start LBA
 *   +32 boot filename
 *
 * directory entry (64 bytes):
 *   +0  name[32]
 *   +32 start LBA
 *   +36 byte length
 *   +40 flags
 */

_start:
    movw    sp, #0xFFFC
    movt    sp, #0x001F

    movw    r10, #:lower16:RESULT
    movt    r10, #:upper16:RESULT

    /* result[0] = stage marker: started */
    movw    r0, #0x3035              /* "50" */
    movt    r0, #0x5453              /* "ST" -> bytes 35 30 53 54 */
    str     r0, [r10, #0]

    /* Read header LBA 0. */
    mov     r0, #0
    movw    r1, #:lower16:HEADER_BUF
    movt    r1, #:upper16:HEADER_BUF
    bl      disk_read_sector
    cmp     r0, #0
    beq     fail_disk

    movw    r4, #:lower16:HEADER_BUF
    movt    r4, #:upper16:HEADER_BUF

    /* Verify "SFLT". */
    ldr     r5, [r4, #0]
    str     r5, [r10, #4]
    movw    r6, #0x4653
    movt    r6, #0x544C              /* 0x544C4653 == "SFLT" LE */
    cmp     r5, r6
    bne     fail_magic

    /* Capture and sanity-check header fields. */
    ldr     r7, [r4, #8]             /* sector size */
    str     r7, [r10, #8]
    cmp     r7, #512
    bne     fail_header

    ldr     r7, [r4, #16]            /* directory LBA */
    str     r7, [r10, #12]
    cmp     r7, #0
    beq     fail_header

    ldr     r8, [r4, #20]            /* directory entries */
    str     r8, [r10, #16]
    cmp     r8, #0
    beq     fail_header

    ldr     r9, [r4, #24]            /* entry size */
    str     r9, [r10, #20]
    cmp     r9, #64
    bne     fail_header

    /* Read the first directory sector using directory LBA FROM HEADER. */
    mov     r0, r7
    movw    r1, #:lower16:DIR_BUF
    movt    r1, #:upper16:DIR_BUF
    bl      disk_read_sector
    cmp     r0, #0
    beq     fail_disk

    /*
     * Scan the eight 64-byte entries in the first directory sector.
     * The host deliberately puts DUMMY.BIN first, so BOOT.BIN is not entry 0.
     */
    movw    r4, #:lower16:DIR_BUF
    movt    r4, #:upper16:DIR_BUF
    mov     r5, #8

scan_entry:
    ldr     r6, [r4, #0]
    movw    r7, #0x4F42
    movt    r7, #0x544F              /* "BOOT" LE = 0x544F4F42 */
    cmp     r6, r7
    bne     next_entry

    ldr     r6, [r4, #4]
    movw    r7, #0x422E
    movt    r7, #0x4E49              /* ".BIN" LE = 0x4E49422E */
    cmp     r6, r7
    bne     next_entry

    /* Found BOOT.BIN. Record exact directory metadata. */
    ldr     r6, [r4, #32]
    str     r6, [r10, #24]           /* BOOT start LBA */
    ldr     r7, [r4, #36]
    str     r7, [r10, #28]           /* BOOT byte length */
    ldr     r8, [r4, #40]
    str     r8, [r10, #32]           /* flags */

    movw    r0, #0x4B4F
    movt    r0, #0x5442              /* bytes "OKBT" */
    str     r0, [r10, #36]
    bkpt    0x1234

next_entry:
    add     r4, r4, #64
    subs    r5, r5, #1
    bne     scan_entry
    b       fail_not_found

/*
 * r0 = LBA
 * r1 = destination
 * returns r0 = 1 success, 0 failure
 *
 * Intentionally mirrors the already-proven 004 BIOS-style MMIO primitive.
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
    str     r0, [r10, #40]
    bkpt    0x1234
