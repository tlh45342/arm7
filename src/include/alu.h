#pragma once
#include "cpu.h"

void handle_add(exec_ctx_t *e);
void handle_adc(exec_ctx_t *e);
void handle_sub(exec_ctx_t *e);
void handle_sbc(exec_ctx_t *e);
void handle_rsb(exec_ctx_t *e);
void handle_rsc(exec_ctx_t *e);
void handle_and(exec_ctx_t *e);
void handle_eor(exec_ctx_t *e);
void handle_mov_dp(exec_ctx_t *e);
void handle_mvn(exec_ctx_t *e);
void handle_cmp(exec_ctx_t *e);
void handle_tst(exec_ctx_t *e);
void handle_teq(exec_ctx_t *e);
void handle_cmn(exec_ctx_t *e);
void handle_umull(exec_ctx_t *e);
void handle_umlal(exec_ctx_t *e);
void handle_smull(exec_ctx_t *e);
void handle_smlal(exec_ctx_t *e);
void handle_mul(exec_ctx_t *e);
void handle_mla(exec_ctx_t *e);
void handle_mvn(exec_ctx_t *e);