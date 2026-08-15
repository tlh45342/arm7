#pragma once
#include <stdint.h>
#include "cpu.h"  // for exec_ctx_t

// Data-processing "logic" operations — all handlers use exec_ctx_t*

// Base logic ops
void handle_and      (exec_ctx_t *e);
void handle_eor      (exec_ctx_t *e);
void handle_tst      (exec_ctx_t *e);
void handle_teq      (exec_ctx_t *e);
void handle_cmp      (exec_ctx_t *e);
void handle_cmn      (exec_ctx_t *e);
void handle_orr      (exec_ctx_t *e);
void handle_mov      (exec_ctx_t *e);
void handle_bic      (exec_ctx_t *e);
void handle_mvn      (exec_ctx_t *e);

// Immediate forms
void handle_tst_imm  (exec_ctx_t *e);
void handle_cmp_imm  (exec_ctx_t *e);
void handle_movw     (exec_ctx_t *e);
void handle_movt     (exec_ctx_t *e);
void handle_mov_imm  (exec_ctx_t *e);
void handle_rsb_imm  (exec_ctx_t *e);
void handle_cmn_imm  (exec_ctx_t *e);

// Byte/half reversals
void handle_rev      (exec_ctx_t *e);
void handle_rev16    (exec_ctx_t *e);
void handle_revsh    (exec_ctx_t *e);

// Sign/zero extend
void handle_uxtb     (exec_ctx_t *e);
void handle_uxth     (exec_ctx_t *e);
void handle_sxtb     (exec_ctx_t *e);
void handle_sxth     (exec_ctx_t *e);


void handle_rbit     (exec_ctx_t *e);