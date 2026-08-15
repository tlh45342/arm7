// src/include/branch.h
#pragma once

#include <stdint.h>

#include "cpu.h"  // for exec_ctx_t

void handle_b        (exec_ctx_t *e);
void handle_bl       (exec_ctx_t *e);
void handle_bx       (exec_ctx_t *e);
void handle_blx_reg  (exec_ctx_t *e);
void handle_blx_imm  (exec_ctx_t *e);