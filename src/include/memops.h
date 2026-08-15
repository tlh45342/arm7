// src/include/memops.h
#pragma once
#include <stdint.h>
#include "cpu.h"  // for exec_ctx_t

// --- Single data transfer (word/byte), immediate forms
void handle_ldr_preimm    (exec_ctx_t *e);
void handle_str_preimm    (exec_ctx_t *e);
void handle_str_predec    (exec_ctx_t *e);
void handle_ldr_postimm   (exec_ctx_t *e);
void handle_str_postimm   (exec_ctx_t *e);

// --- Register offset / shifted register forms
void handle_ldr_regoffset (exec_ctx_t *e);
void handle_str_regoffset (exec_ctx_t *e);
void handle_ldrb_reg      (exec_ctx_t *e);
void handle_ldrb_reg_shift(exec_ctx_t *e);

// --- Byte transfers, immediate forms
void handle_ldrb_preimm   (exec_ctx_t *e);
void handle_ldrb_postimm  (exec_ctx_t *e);
void handle_strb_preimm   (exec_ctx_t *e);
void handle_strb_postimm  (exec_ctx_t *e);
void handle_strb_reg_shift(exec_ctx_t *e);

// --- Literal
void handle_ldr_literal   (exec_ctx_t *e);

// --- Halfword / signed variants
void handle_strh          (exec_ctx_t *e);
void handle_ldrh          (exec_ctx_t *e);
void handle_ldrsb         (exec_ctx_t *e);
void handle_ldrsh         (exec_ctx_t *e);

// --- Block transfers
void handle_ldm           (exec_ctx_t *e);
void handle_stm           (exec_ctx_t *e);

// --- Doubleword transfers
void exec_ldrd_imm        (exec_ctx_t *e);
void exec_ldrd_reg        (exec_ctx_t *e);
void exec_strd_imm        (exec_ctx_t *e);
void exec_strd_reg        (exec_ctx_t *e);

// --- Stack helpers
void handle_pop_pc        (exec_ctx_t *e);

// --- Swap
void handle_swp           (exec_ctx_t *e);
void handle_swpb          (exec_ctx_t *e);