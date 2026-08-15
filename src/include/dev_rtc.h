// src/include/dev_rtc.h
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Register map (byte offsets from RTC_BASE_ADDR)
//  0x00: SECONDS    (RO) Unix time_t seconds since 1970-01-01
//  0x04: MILLIS     (RO) 0..999 (best-effort)
//  0x08: CTRL       (RW) bit0=FREEZE (1=freeze snapshot), bit1=WENA (write-enable)
//  0x0C: STATUS     (RO) bit0=FROZEN (latched)
//  0x10: YEAR       (RO) e.g. 2025
//  0x14: MONTH      (RO) 1..12
//  0x18: DAY        (RO) 1..31
//  0x1C: HOUR       (RO) 0..23
//  0x20: MIN        (RO) 0..59
//  0x24: SEC        (RO) 0..60 (leap)
//  0x28: SET_SECS   (WO) write seconds here when CTRL.WENA=1 to set the clock
//  0x2C: CLEAR      (WO) write 1: unfreeze & clear WENA

// Choose a base that doesn't collide with your other devices
#define RTC_BASE_ADDR  0xF0002000u
#define RTC_MMIO_SIZE  0x00000100u

// Initialize the device (set base address, prime a snapshot)
void     dev_rtc_init(uint32_t base_addr);

// MMIO handlers you hook into your bus
uint32_t dev_rtc_read32(uint32_t addr);
void     dev_rtc_write32(uint32_t addr, uint32_t value);

#ifdef __cplusplus
}
#endif