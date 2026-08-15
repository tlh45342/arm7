#ifndef ARM7_COPROC_H
#define ARM7_COPROC_H

#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"

typedef struct arm7_coproc_xfer64 {
    uint8_t cp;
    uint8_t opc1;
    uint8_t crm;
    uint8_t rt;
    uint8_t rt2;
    bool is_cp2;
} arm7_coproc_xfer64_t;

bool arm7_coproc_read64(exec_ctx_t *e,
                        const arm7_coproc_xfer64_t *xfer,
                        uint64_t *value);

bool arm7_coproc_write64(exec_ctx_t *e,
                         const arm7_coproc_xfer64_t *xfer,
                         uint64_t value);

void handle_mrrc(exec_ctx_t *e);
void handle_mrrc2(exec_ctx_t *e);
void handle_mcrr(exec_ctx_t *e);
void handle_mcrr2(exec_ctx_t *e);

#endif
