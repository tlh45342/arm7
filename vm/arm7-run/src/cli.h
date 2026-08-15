#ifndef ARM7_RUN_CLI_H
#define ARM7_RUN_CLI_H

#include <stdbool.h>
#include <stdio.h>

#include "arm7vm/arm7vm.h"

typedef struct CLI {
    arm7vm_machine_t *machine;
    FILE *in;
    bool interactive;
} CLI;

void cli_init(
    CLI *cli,
    arm7vm_machine_t *machine,
    FILE *in,
    bool interactive
);

int cli_eval_line(CLI *cli, const char *line);
int cli_run(CLI *cli);

#endif
