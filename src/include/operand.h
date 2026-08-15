#pragma once
#include <stdint.h>
#include "cpu.h"

// Context-aware Operand2 decoder.
// - e: current execution context
// - instr: full 32-bit instruction word
// - sh_carry: in/out, carries the incoming C flag and receives the shifter carry-out
uint32_t dp_operand2_ctx(exec_ctx_t *e, uint32_t instr, uint32_t *sh_carry);

// Compat macro: many handlers already call dp_operand2(instr,&c) with `e` in scope.
// This macro forwards to the ctx-aware version without rewriting all call sites today.
#define dp_operand2(instr, sh_carry_ptr) dp_operand2_ctx(e, (instr), (sh_carry_ptr))
