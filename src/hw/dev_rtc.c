// src/hw/dev_rtc.c
#include "dev_rtc.h"
#include <time.h>
#include <string.h>

typedef struct {
    uint32_t base;
    // Latched snapshot when FREEZE=1
    struct tm latched_tm;
    time_t    latched_secs;
    int       latched_ms;
    // Control
    uint32_t  ctrl;   // bit0 FREEZE, bit1 WENA
} rtc_dev_t;

static rtc_dev_t g_rtc = {0};

static void rtc_get_now(time_t* out_secs, struct tm* out_tm, int* out_ms) {
    // Epoch seconds
    time_t s = time(NULL);

    // Best-effort milliseconds (portable fallback)
    int ms = (int)((clock() * 1000.0) / (double)CLOCKS_PER_SEC);
    if (ms < 0) { ms = 0; }
    ms %= 1000;

    // Local time (use gmtime_r/s for UTC if you prefer)
#if defined(_WIN32)
    struct tm tmlocal;
    localtime_s(&tmlocal, &s);
    if (out_tm) *out_tm = tmlocal;
#else
    struct tm tmlocal;
    localtime_r(&s, &tmlocal);
    if (out_tm) *out_tm = tmlocal;
#endif
    if (out_secs) *out_secs = s;
    if (out_ms)   *out_ms   = ms;
}

void dev_rtc_init(uint32_t base_addr) {
    memset(&g_rtc, 0, sizeof(g_rtc));
    g_rtc.base = base_addr;
    // Prime a snapshot so reads work even if frozen immediately
    rtc_get_now(&g_rtc.latched_secs, &g_rtc.latched_tm, &g_rtc.latched_ms);
}

static inline uint32_t ro(uint32_t v){ return v; }

uint32_t dev_rtc_read32(uint32_t addr) {
    uint32_t off = addr - g_rtc.base;

    time_t secs; struct tm t; int ms;
    if (g_rtc.ctrl & 0x1u) {            // FREEZE
        secs = g_rtc.latched_secs;
        t    = g_rtc.latched_tm;
        ms   = g_rtc.latched_ms;
    } else {
        rtc_get_now(&secs, &t, &ms);
    }

    switch (off) {
        case 0x00: return ro((uint32_t)secs);                      // SECONDS
        case 0x04: return ro((uint32_t)ms);                        // MILLIS
        case 0x08: return ro(g_rtc.ctrl);                          // CTRL
        case 0x0C: return ro((g_rtc.ctrl & 0x1u) ? 1u : 0u);       // STATUS
        case 0x10: return ro((uint32_t)(t.tm_year + 1900));        // YEAR
        case 0x14: return ro((uint32_t)(t.tm_mon + 1));            // MONTH
        case 0x18: return ro((uint32_t)(t.tm_mday));               // DAY
        case 0x1C: return ro((uint32_t)(t.tm_hour));               // HOUR
        case 0x20: return ro((uint32_t)(t.tm_min));                // MIN
        case 0x24: return ro((uint32_t)(t.tm_sec));                // SEC
        default:   return 0u;
    }
}

void dev_rtc_write32(uint32_t addr, uint32_t value) {
    uint32_t off = addr - g_rtc.base;

    switch (off) {
        case 0x08: { // CTRL: bit0 FREEZE, bit1 WENA
            uint32_t prev = g_rtc.ctrl;
            g_rtc.ctrl = (value & 0x3u);
            // If FREEZE rising edge, latch a snapshot
            if (((g_rtc.ctrl ^ prev) & 0x1u) && (g_rtc.ctrl & 0x1u)) {
                rtc_get_now(&g_rtc.latched_secs, &g_rtc.latched_tm, &g_rtc.latched_ms);
            }
            break;
        }

        case 0x28: { // SET_SECS (WO): valid only when CTRL.WENA=1
            if (g_rtc.ctrl & 0x2u) {
                time_t s = (time_t)value;
#if defined(_WIN32)
                struct tm t; localtime_s(&t, &s);
#else
                struct tm t; localtime_r(&s, &t);
#endif
                g_rtc.latched_secs = s;
                g_rtc.latched_tm   = t;
                g_rtc.latched_ms   = 0;
                // Freeze so subsequent reads are consistent
                g_rtc.ctrl |= 0x1u; // FREEZE
            }
            break;
        }

        case 0x2C: { // CLEAR (WO): write 1 => unfreeze & clear WENA
            if (value & 1u) {
                g_rtc.ctrl &= ~(0x1u); // FREEZE=0
                g_rtc.ctrl &= ~(0x2u); // WENA=0
            }
            break;
        }

        default:
            // Ignore writes to RO space
            break;
    }
}