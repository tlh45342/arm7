// include/execute.h
#pragma once

#include "cpu.h"   // for exec_ctx_t

// Decoder/dispatcher handler signature used by the K12 table
typedef void (*k12_fn_t)(exec_ctx_t *);

// New core entry
bool cpu_execute(exec_ctx_t *e);