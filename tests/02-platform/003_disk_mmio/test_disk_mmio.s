.syntax unified
.arch armv7-a
.arm

/*
 * ARM7 disk MMIO platform test.
 *
 * Current disk0 PIO contract:
 *
 *   0x0B000004  LBA
 *   0x0B000008  COUNT
 *   0x0B00000C  CMD
 *   0x0B000010  STATUS
 *   0x0B000200  DATA[0..511]
 *
 * CMD:
 *   bit 7 GO
 *   bit 0 READ
 *   READ|GO = 0x81
 *
 * STATUS:
 *   bit 0 BUSY
 *   bit 1 DRQ
 *   bit 2 ERR
 *
 * The host harness creates LBA 0 beginning with:
 *   "ARM7DISK-TEST-0001"
 *
 * This guest copies the first 32 bytes from the DATA window into ordinary
 * RAM at 0x00110000, stores final STATUS at 0x00110020, then BKPTs.
 */

.equ DISK_BASE,    0x0B000000
.equ DISK_LBA,     0x04
.equ DISK_COUNT,   0x08
.equ DISK_CMD,     0x0C
.equ DISK_STATUS,  0x10
.equ DISK_DATA,    0x200

.equ CMD_READ_GO,  0x81
.equ ST_BUSY,      0x01
.equ ST_DRQ,       0x02
.equ ST_ERR,       0x04

.equ RESULTS,      0x00110000

.text
.global _start

_start:
    movw    r0, #:lower16:DISK_BASE
    movt    r0, #:upper16:DISK_BASE

    /* LBA = 0 */
    mov     r1, #0
    str     r1, [r0, #DISK_LBA]

    /* COUNT = 1 */
    mov     r1, #1
    str     r1, [r0, #DISK_COUNT]

    /* CMD = READ | GO */
    mov     r1, #CMD_READ_GO
    str     r1, [r0, #DISK_CMD]

wait_ready:
    ldr     r2, [r0, #DISK_STATUS]

    /* Error -> record status and stop. */
    tst     r2, #ST_ERR
    bne     record_status

    /* Busy -> keep waiting. */
    tst     r2, #ST_BUSY
    bne     wait_ready

    /* Need DRQ before consuming DATA. */
    tst     r2, #ST_DRQ
    beq     wait_ready

    /*
     * Copy first 32 bytes (8 words) from the 512-byte PIO DATA window.
     * This proves the device MMIO window, not a host-side shortcut.
     */
    movw    r3, #:lower16:RESULTS
    movt    r3, #:upper16:RESULTS

    ldr     r4, [r0, #(DISK_DATA + 0x00)]
    str     r4, [r3, #0x00]
    ldr     r4, [r0, #(DISK_DATA + 0x04)]
    str     r4, [r3, #0x04]
    ldr     r4, [r0, #(DISK_DATA + 0x08)]
    str     r4, [r3, #0x08]
    ldr     r4, [r0, #(DISK_DATA + 0x0C)]
    str     r4, [r3, #0x0C]
    ldr     r4, [r0, #(DISK_DATA + 0x10)]
    str     r4, [r3, #0x10]
    ldr     r4, [r0, #(DISK_DATA + 0x14)]
    str     r4, [r3, #0x14]
    ldr     r4, [r0, #(DISK_DATA + 0x18)]
    str     r4, [r3, #0x18]
    ldr     r4, [r0, #(DISK_DATA + 0x1C)]
    str     r4, [r3, #0x1C]

record_status:
    movw    r3, #:lower16:RESULTS
    movt    r3, #:upper16:RESULTS
    str     r2, [r3, #0x20]

    bkpt    #0x444B
