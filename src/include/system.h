// src/include/system.h
#pragma once
#include <stdint.h>

// Misc/system & hints
void handle_bkpt      (exec_ctx_t *e);
void handle_cps       (exec_ctx_t *e);
void handle_msr       (exec_ctx_t *e);
void handle_mrs       (exec_ctx_t *e);
void handle_svc       (exec_ctx_t *e);
void handle_nop       (exec_ctx_t *e);
void handle_wfi       (exec_ctx_t *e);
void handle_dsb       (exec_ctx_t *e);
void handle_dmb       (exec_ctx_t *e);
void handle_isb       (exec_ctx_t *e);

// Bitfield & count
void handle_bfc       (exec_ctx_t *e);
void handle_bfi       (exec_ctx_t *e);
void handle_clz       (exec_ctx_t *e);

// Wide moves
void handle_movw      (exec_ctx_t *e);
void handle_movt      (exec_ctx_t *e);

// Easter egg / halt
void handle_deadbeef  (exec_ctx_t *e);