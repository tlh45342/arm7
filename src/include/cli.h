#pragma once
#include <stdio.h>
#include <stdbool.h>
#include "vm.h"

typedef struct {
    VM   *vm;
    FILE *in;   // usually stdin or a script file
    bool  interactive;
} CLI;

void cli_init(CLI *cli, VM *vm, FILE *in, bool interactive);
int  cli_run(CLI *cli);                  // REPL: returns 0 on normal quit
int  cli_eval_line(CLI *cli, const char *line); // for scripted input