#pragma once
#include <stdint.h>

// Adjust if your RAM base differs
#define RAM_BASE   0x00000000u

// Place DTB at a safe, aligned spot in RAM (8-byte aligned)
#define DTB_ADDR   0x00040000u  // 256 KiB into RAM