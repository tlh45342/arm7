   .syntax unified
        .cpu cortex-a15
        .arch armv7-a
        .text
        .global _start

.equ RTC_BASE,   0xF0002000
.equ RTC_SEC,    (RTC_BASE + 0x00)
.equ RTC_MS,     (RTC_BASE + 0x04)
.equ RTC_CTRL,   (RTC_BASE + 0x08)
.equ RTC_STAT,   (RTC_BASE + 0x0C)
.equ RTC_YEAR,   (RTC_BASE + 0x10)
.equ RTC_MON,    (RTC_BASE + 0x14)
.equ RTC_DAY,    (RTC_BASE + 0x18)
.equ RTC_HOUR,   (RTC_BASE + 0x1C)
.equ RTC_MIN,    (RTC_BASE + 0x20)
.equ RTC_SEC_F,  (RTC_BASE + 0x24)

_start:
        // r6 = results buffer at 0x0010_0000 (same pattern as other tests)
        movw    r6, #0
        movt    r6, #0x0010

        // Read current time
        ldr     r0, =RTC_YEAR
        ldr     r1, [r0]          // year
        str     r1, [r6, #0]
        ldr     r1, [r0, #0x04]   // month
        str     r1, [r6, #4]
        ldr     r1, [r0, #0x08]   // day
        str     r1, [r6, #8]
        ldr     r1, [r0, #0x0C]   // hour
        str     r1, [r6, #12]
        ldr     r1, [r0, #0x10]   // min
        str     r1, [r6, #16]
        ldr     r1, [r0, #0x14]   // sec
        str     r1, [r6, #20]

        // Seconds + millis
        ldr     r2, =RTC_SEC
        ldr     r3, [r2]
        str     r3, [r6, #24]
        ldr     r3, [r2, #4]
        str     r3, [r6, #28]

        // Freeze snapshot
        ldr     r4, =RTC_CTRL
        mov     r5, #1            // FREEZE=1
        str     r5, [r4]

        // Read again into next slots (should be identical while frozen)
        ldr     r1, [r0]          // year (frozen)
        str     r1, [r6, #32]
        ldr     r1, [r0, #0x14]   // sec (frozen)
        str     r1, [r6, #36]
        ldr     r3, [r2]          // seconds (frozen)
        str     r3, [r6, #40]

        // Unfreeze and clear WENA in one shot
        ldr     r7, =RTC_BASE
        mov     r5, #1
        str     r5, [r7, #0x2C]   // CLEAR

        // BKPT
        .word   0xE1200070
		