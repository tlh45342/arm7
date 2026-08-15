// =============================
// File: src/include/bitutil.h
// =============================
#pragma once
#include <stdint.h>


static inline uint32_t ror32(uint32_t x, unsigned n) {
n &= 31u;
return (x >> n) | (x << (32 - n));
}


static inline uint32_t asr32_u(uint32_t x, unsigned n) {
if (n == 0) return x;
if (n >= 32) return (x & 0x80000000u) ? 0xFFFFFFFFu : 0;
uint32_t signmask = (x & 0x80000000u) ? (~0u << (32 - n)) : 0u;
return (x >> n) | signmask;
}


static inline int32_t sign_extend_width(uint32_t v, unsigned width) {
if (width == 0 || width >= 32) return (int32_t)v;
uint32_t m = 1u << (width - 1);
uint32_t mask = (1u << width) - 1u;
v &= mask;
return (int32_t)((v ^ m) - m);
}


static inline uint32_t zero_extend_width(uint32_t v, unsigned width) {
if (width >= 32) return v;
return v & ((1u << width) - 1u);
}