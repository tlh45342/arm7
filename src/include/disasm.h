// src/include/disasm.h
#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Canonical disasm line API (pc first, then instr)
void disasm_line(uint32_t pc, uint32_t instr, char *out, size_t out_sz);

// Compatibility wrapper some codebases use (instr first, then pc)
void a32_disasm(uint32_t instr, uint32_t pc, char *out, size_t out_sz);

