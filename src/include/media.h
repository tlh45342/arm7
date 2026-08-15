// File: src/include/media.h
// ============================
#pragma once
#include <stdint.h>


// Group decoders (K12 shims). These keep your table simple.
void handle_group_bitfield_saturate(uint32_t instr); // SBFX/UBFX + SSAT/USAT/SSAT16/USAT16
void handle_group_extend(uint32_t instr); // SXTB/SXTB16/SXTH/UXTB/UXTB16/UXTH


// Individual handlers (lowercase, void return) — can be called directly too.
void handle_sbfx(exec_ctx_t *e);
void handle_ubfx(exec_ctx_t *e);
void handle_ssat(exec_ctx_t *e);
void handle_usat(exec_ctx_t *e);
void handle_ssat16(exec_ctx_t *e);
void handle_usat16(exec_ctx_t *e);

void handle_sxtb16(uint32_t instr);