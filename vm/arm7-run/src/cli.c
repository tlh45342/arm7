// vm/arm7-run/src/cli.c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "version.h"
#include "cli.h"
#include "log.h"
#include "debug.h"

static int ieq(const char* a, const char* b) {
    while (*a && *b) { if (tolower((unsigned char)*a++) != tolower((unsigned char)*b++)) return 0; }
    return *a == '\0' && *b == '\0';
}

static debug_flags_t debug_name_to_bit(const char* name) {
    if (ieq(name, "INSTR"))     return DBG_INSTR;
    if (ieq(name, "MEM_READ"))  return DBG_MEM_READ;
    if (ieq(name, "MEM_WRITE")) return DBG_MEM_WRITE;
    if (ieq(name, "MMIO"))      return DBG_MMIO;
    if (ieq(name, "DISK"))      return DBG_DISK;
    if (ieq(name, "IRQ"))       return DBG_IRQ;
    if (ieq(name, "DISASM"))    return DBG_DISASM;
    if (ieq(name, "K12"))       return DBG_K12;
    if (ieq(name, "TRACE"))     return DBG_TRACE;
    if (ieq(name, "CLI"))       return DBG_CLI;
    if (ieq(name, "ALL"))       return DBG_ALL;
    if (ieq(name, "NONE"))      return DBG_NONE;
    return 0;
}

static debug_flags_t parse_debug_expr(const char* expr) {
    if (!expr) return DBG_NONE;

    while (isspace((unsigned char)*expr)) expr++;
    if (expr[0] == '0' && (expr[1] == 'x' || expr[1] == 'X')) {
        char *endp = NULL;
        unsigned long v = strtoul(expr, &endp, 16);
        if (endp && *endp == '\0') return (debug_flags_t)v;
    } else if (isdigit((unsigned char)expr[0])) {
        char *endp = NULL;
        unsigned long v = strtoul(expr, &endp, 10);
        if (endp && *endp == '\0') return (debug_flags_t)v;
    }

    debug_flags_t out = 0;
    const char *p = expr;

    while (*p) {
        while (*p && (isspace((unsigned char)*p) || *p == ',' || *p == '|')) p++;
        if (!*p) break;

        int sign = +1;
        if (*p == '+') { sign = +1; ++p; }
        else if (*p == '-') { sign = -1; ++p; }

        char tok[32];
        size_t tn = 0;
        while (*p && !isspace((unsigned char)*p) && *p != ',' && *p != '|' &&
               *p != '+' && *p != '-' && tn < sizeof(tok) - 1) {
            tok[tn++] = *p++;
        }
        tok[tn] = '\0';
        if (tn == 0) continue;

        debug_flags_t f = debug_name_to_bit(tok);
        if (!f && (tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X'))) {
            char *e = NULL; unsigned long v = strtoul(tok, &e, 16);
            if (e && *e == '\0') f = (debug_flags_t)v;
        } else if (!f && isdigit((unsigned char)tok[0])) {
            char *e = NULL; unsigned long v = strtoul(tok, &e, 10);
            if (e && *e == '\0') f = (debug_flags_t)v;
        }

        if (ieq(tok, "NONE")) {
            if (sign > 0) out = 0;
            else out = DBG_ALL;
        } else if (sign > 0) {
            out |= f;
        } else {
            out &= ~f;
        }
    }

    return out;
}

static const char* find_eq_rhs(const char* token, const char* next_or_null) {
    const char *eq = strchr(token, '=');
    if (eq && *(eq + 1)) return eq + 1;
    return next_or_null;
}

extern debug_flags_t debug_flags;

typedef int (*cmd_fn)(CLI*, int argc, char **argv);
typedef struct { const char *name; cmd_fn fn; const char *help; } cmd_t;

static int cmd_run     (CLI*, int, char**);
static int cmd_regs    (CLI*, int, char**);
static int cmd_load    (CLI*, int, char**);
static int cmd_logfile (CLI*, int, char**);
static int cmd_do      (CLI*, int, char**);
static int cmd_quit    (CLI*, int, char**);
static int cmd_attach  (CLI*, int, char **);
static int cmd_set     (CLI*, int, char**);
static int cmd_clrhalt (CLI*, int, char**);
static int cmd_step    (CLI*, int, char**);
static int cmd_help    (CLI*, int, char**);
static int cmd_show    (CLI*, int, char**);
static int cmd_version (CLI*, int, char**);
static int cmd_examine (CLI*, int, char**);
static int cmd_key     (CLI*, int, char**);

static const cmd_t CMDS[] = {
    {"run",      cmd_run,     "Run until halt"},
    {"regs",     cmd_regs,    "Dump registers"},
    {"help",     cmd_help,    "show command help"},
    {"show",     cmd_show,    "show crt | show help"},
    {"load",     cmd_load,    "load <bin> <addr>"},
    {"key",      cmd_key,     "key <byte|char> (inject keyboard byte)"},
    {"e",        cmd_examine, "examine memory (e addr[-end])"},
    {"clrhalt",  cmd_clrhalt, "clear CPU halt"},
    {"step",     cmd_step,    "step [N] (default 1)"},
    {"attach",   cmd_attach,  "attach disk0 <image>"},
    {"version",  cmd_version, "show emulator version"},
    {"logfile",  cmd_logfile, "logfile <path>"},
    {"do",       cmd_do,      "do <scriptfile>"},
    {"set",      cmd_set,     "set rN <val> | set pc <val> | set cpu debug=<flag>"},
    {"quit",     cmd_quit,    "Exit"},
};

static int cmd_clrhalt(CLI *cli, int argc, char **argv) {
    (void)argc; (void)argv;
    arm7vm_clear_halt(cli->machine);
    log_printf("[CPU] halt cleared\n");
    return 0;
}

static int cmd_version(CLI *cli, int argc, char **argv) {
    (void)cli; (void)argc; (void)argv;
    log_printf("arm7-run version %s\n", VERSION);
    return 0;
}

static void strip_inline_comments(char *s) {
    if (!s) return;
    char *p = s;

    for (char *q = s; *q; ++q) {
        if (q[0] == '/' && q[1] == '/') { *q = '\0'; break; }
    }

    for (char *q = s; *q; ++q) {
        if (*q == ';' || *q == '#') { *q = '\0'; break; }
    }

    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n-1])) s[--n] = '\0';

    while (isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

static bool parse_u32(const char *s, uint32_t *out) {
    if (!s || !*s) return false;
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 0);
    if (end == s || *end != '\0' || v > 0xFFFFFFFFul) return false;
    *out = (uint32_t)v;
    return true;
}

static int cmd_help(CLI *cli, int argc, char **argv) {
    (void)cli; (void)argc; (void)argv;

    log_printf("arm7-run commands:\n");
    for (size_t i = 0; i < sizeof(CMDS) / sizeof(CMDS[0]); ++i)
        log_printf("  %-8s %s\n", CMDS[i].name, CMDS[i].help);
    return 0;
}

static void show_crt(CLI *cli) {
    enum {
        CRT_BASE = 0x0A000000u,
        CRT_COLS = 80,
        CRT_ROWS = 25
    };

    log_printf("\n--- CRT 80x25 @ 0x%08X ---\n", CRT_BASE);

    for (int row = 0; row < CRT_ROWS; row++) {
        for (int col = 0; col < CRT_COLS; col++) {
            uint32_t addr = CRT_BASE + (uint32_t)((row * CRT_COLS + col) * 2);
            uint8_t ch = 0;

            if (!arm7vm_read_memory(cli->machine, addr, &ch, 1))
                ch = ' ';

            if (ch < 32 || ch > 126)
                ch = ' ';

            log_printf("%c", ch);
        }
        log_printf("\n");
    }

    log_printf("--- end CRT ---\n");
}

static int cmd_show(CLI *cli, int argc, char **argv) {
    if (argc < 2 || ieq(argv[1], "help")) {
        log_printf("Usage:\n");
        log_printf("  show crt\n");
        log_printf("  show help\n");
        return 0;
    }

    if (ieq(argv[1], "crt")) {
        show_crt(cli);
        return 0;
    }

    log_printf("[ERROR] unknown show target: %s\n", argv[1]);
    log_printf("Try: show help\n");
    return -1;
}

static int cmd_examine(CLI *cli, int argc, char **argv) {
    if (!cli || !cli->machine) { log_printf("No VM.\n"); return 1; }

    if (argc < 2) {
        log_printf("Usage: e <addr>[-<end>]\n");
        return 1;
    }

    char buf[128];
    strncpy(buf, argv[1], sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    uint32_t start = 0, end = 0;
    char *dash = strchr(buf, '-');

    if (dash) {
        *dash = '\0';
        if (!parse_u32(buf, &start) || !parse_u32(dash + 1, &end)) {
            log_printf("e: invalid range '%s-%s'\n", buf, dash + 1);
            return 1;
        }
    } else {
        if (!parse_u32(buf, &start)) {
            log_printf("e: invalid address '%s'\n", buf);
            return 1;
        }
        end = start;
    }

    if (end < start) { uint32_t t = start; start = end; end = t; }
    size_t count = (size_t)(end - start + 1);

    if (count <= 16) {
        log_printf("0x%08x:", start);
        for (uint32_t a = start; a <= end; ++a) {
            uint8_t b = 0;
            if (arm7vm_read_memory(cli->machine, a, &b, 1))
                log_printf(" %02x", (unsigned)b);
            else
                log_printf(" ??");
        }
        log_printf("\n");
        return 0;
    }

    uint32_t a = start;
    while (a <= end) {
        log_printf("0x%08x:", a);

        for (int i = 0; i < 16 && a <= end; ++i, ++a) {
            uint8_t b = 0;
            if (arm7vm_read_memory(cli->machine, a, &b, 1))
                log_printf(" %02x", (unsigned)b);
            else
                log_printf(" ??");
        }
        log_printf("\n");
    }

    return 0;
}

static int cmd_attach(CLI *cli, int argc, char **argv) {
    if (argc < 3) { log_printf("usage: attach disk0 <image>\n"); return -1; }
    if (strcmp(argv[1], "disk0") != 0) {
        log_printf("[ERROR] unknown device: %s\n", argv[1]);
        return -1;
    }
    if (!arm7vm_attach_disk0(cli->machine, argv[2])) return -1;
    return 0;
}

void cli_init(CLI *cli, arm7vm_machine_t *machine, FILE *in, bool interactive) {
    cli->machine = machine;
    cli->in = in;
    cli->interactive = interactive;
}

static int tokenize(char *line, char **argv, int maxv) {
    int n = 0;
    char *p = strtok(line, " \t\r\n");
    while (p && n < maxv) { argv[n++] = p; p = strtok(NULL, " \t\r\n"); }
    return n;
}

static bool is_comment_or_blank(const char *s) {
    while (*s == ' ' || *s == '\t') ++s;
    return (*s == 0) || (*s == '#') || (*s == ';') ||
           (s[0] == '/' && s[1] == '/');
}

int cli_eval_line(CLI *cli, const char *line_in) {
    char buf[1024];

    strncpy(buf, line_in, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    strip_inline_comments(buf);
    if (is_comment_or_blank(buf)) return 0;

    char *argv[32];
    int argc = tokenize(buf, argv, 32);
    if (!argc) return 0;

    for (size_t i = 0; i < sizeof(CMDS)/sizeof(CMDS[0]); ++i) {
        if (strcmp(argv[0], CMDS[i].name) == 0)
            return CMDS[i].fn(cli, argc, argv);
    }

    log_printf("[ERROR] Unknown command: %s\n", argv[0]);
    return -1;
}

int cli_run(CLI *cli) {
    char line[1024];

    for (;;) {
        if (cli->interactive) { fputs("arm7-run> ", stdout); fflush(stdout); }
        if (!fgets(line, sizeof(line), cli->in)) break;

        int rc = cli_eval_line(cli, line);
        if (rc == 1) break;
    }

    return 0;
}

static int cmd_step(CLI *cli, int argc, char **argv) {
    unsigned long n = 1;

    if (argc >= 2) {
        char *endp = NULL;
        n = strtoul(argv[1], &endp, 0);
        if (endp && *endp) { log_printf("usage: step [N]\n"); return -1; }
        if (n == 0) n = 1;
    }

    for (unsigned long i = 0; i < n; ++i) {
        if (!arm7vm_step(cli->machine))
            break;
    }

    return 0;
}

static int cmd_run(CLI *cli, int argc, char **argv) {
    (void)argc; (void)argv;
    DBG("[cmd_run]");
    arm7vm_run(cli->machine, 0);
    return 0;
}

static int cmd_regs(CLI *cli, int argc, char **argv) {
    (void)argc; (void)argv;
    arm7vm_dump_registers(cli->machine);
    return 0;
}

static int cmd_load(CLI *cli, int argc, char **argv) {
    if (argc < 3) {
        log_printf("usage: load <path> <addr>\n");
        return -1;
    }

    const char *path = argv[1];
    uint32_t addr = (uint32_t)strtoul(argv[2], NULL, 0);

    if (!arm7vm_load_binary(cli->machine, path, addr)) {
        log_printf("[ERROR] load failed\n");
        return -1;
    }

    return 0;
}

static int cmd_key(CLI *cli, int argc, char **argv) {
    uint32_t value = 0;
    uint8_t ch;

    if (argc != 2) {
        log_printf("usage: key <byte|char>\n");
        log_printf("examples: key 0x41 | key 65 | key A\n");
        return -1;
    }

    /*
     * A single character may be supplied directly.  Otherwise parse
     * the argument numerically, so scripts can use hex or decimal.
     */
    if (argv[1][0] != '\0' && argv[1][1] == '\0') {
        ch = (uint8_t)(unsigned char)argv[1][0];
    } else {
        if (!parse_u32(argv[1], &value) || value > 0xFFu) {
            log_printf("[ERROR] key expects one character or a byte value 0..255\n");
            return -1;
        }
        ch = (uint8_t)value;
    }

    if (!arm7vm_keyboard_push(cli->machine, ch)) {
        log_printf("[ERROR] keyboard FIFO full; key 0x%02X not queued\n",
                   (unsigned)ch);
        return -1;
    }

    if (ch >= 32u && ch <= 126u) {
        log_printf("[KEY] queued 0x%02X ('%c') pending=%u\n",
                   (unsigned)ch,
                   (char)ch,
                   arm7vm_keyboard_pending(cli->machine));
    } else {
        log_printf("[KEY] queued 0x%02X pending=%u\n",
                   (unsigned)ch,
                   arm7vm_keyboard_pending(cli->machine));
    }

    return 0;
}

static int cmd_logfile(CLI *cli, int argc, char **argv) {
    (void)cli;
    if (argc < 2) { log_printf("usage: logfile <path>\n"); return -1; }
    log_set_file(argv[1]);
    log_printf("Logging to %s\n", argv[1]);
    return 0;
}

static int cmd_do(CLI *cli, int argc, char **argv) {
    if (argc < 2) { log_printf("usage: do <script>\n"); return -1; }

    FILE *f = fopen(argv[1], "r");
    if (!f) { log_printf("[ERROR] cannot open %s\n", argv[1]); return -1; }

    CLI script_cli;
    cli_init(&script_cli, cli->machine, f, false);
    cli_run(&script_cli);
    fclose(f);
    return 0;
}

static int cmd_quit(CLI *cli, int argc, char **argv) {
    (void)cli; (void)argc; (void)argv;
    return 1;
}

static int istarts_with(const char* s, const char* prefix) {
    while (*prefix && *s) {
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix)) return 0;
        ++s; ++prefix;
    }
    return *prefix == '\0';
}

static bool parse_size_arg(const char *s, size_t *out_bytes) {
    if (!s || !*s || !out_bytes) return false;

    char buf[64];
    size_t n = 0;
    while (s[n] && n < sizeof(buf) - 1 && !isspace((unsigned char)s[n])) {
        buf[n] = s[n];
        n++;
    }
    buf[n] = '\0';

    char unit = '\0';
    size_t len = strlen(buf);
    if (len > 0) {
        char last = buf[len - 1];
        if (last == 'K' || last == 'k' || last == 'M' || last == 'm' ||
            last == 'G' || last == 'g') {
            unit = (char)tolower((unsigned char)last);
            buf[len - 1] = '\0';
        }
    }

    char *endp = NULL;
    unsigned long long base = 10;
    if (buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X')) base = 16;

    unsigned long long val = strtoull(buf, &endp, (int)base);
    if (!buf[0] || (endp && *endp)) return false;

    unsigned long long bytes = val;
    if (unit == 'k') bytes *= 1024ull;
    else if (unit == 'm') bytes *= 1024ull * 1024ull;
    else if (unit == 'g') bytes *= 1024ull * 1024ull * 1024ull;

    *out_bytes = (size_t)bytes;
    return true;
}

static int cmd_set(CLI *cli, int argc, char **argv) {
    if (!cli || !cli->machine) { log_printf("No VM.\n"); return -1; }

    if (argc < 2) {
        log_printf("Usage:\n");
        log_printf("  set rN <value>            (N=0..15)\n");
        log_printf("  set pc <value>\n");
        log_printf("  set mem <bytes|K|M|G>\n");
        log_printf("  set debug=<hex|names>\n");
        log_printf("  set cpu debug=<hex|names>\n");
        log_printf("  set trace=on|off\n");
        log_printf("Names: INSTR, MEM_READ, MEM_WRITE, MMIO, DISK, IRQ, DISASM, K12, TRACE, CLI, ALL, NONE\n");
        return 0;
    }

    if (ieq(argv[1], "pc")) {
        if (argc < 3) { log_printf("Usage: set pc <value>\n"); return -1; }

        uint32_t v;
        if (!parse_u32(argv[2], &v)) {
            log_printf("set pc: invalid value '%s'\n", argv[2]);
            return -1;
        }

        arm7vm_set_register(cli->machine, 15, v);
        if (debug_flags & DBG_CLI)
            log_printf("pc <= 0x%08X\n", v);
        return 0;
    }

    if ((argv[1][0] == 'r' || argv[1][0] == 'R') &&
        isdigit((unsigned char)argv[1][1])) {
        char *endp = NULL;
        long reg = strtol(argv[1] + 1, &endp, 10);

        if (*endp || reg < 0 || reg > 15) {
            log_printf("set rN: N must be 0..15\n");
            return -1;
        }
        if (argc < 3) {
            log_printf("Usage: set r%ld <value>\n", reg);
            return -1;
        }

        uint32_t v;
        if (!parse_u32(argv[2], &v)) {
            log_printf("set r%ld: invalid value '%s'\n", reg, argv[2]);
            return -1;
        }

        arm7vm_set_register(cli->machine, (unsigned)reg, v);
        if (debug_flags & DBG_CLI)
            log_printf("r%ld <= 0x%08X\n", reg, v);
        return 0;
    }

    if (ieq(argv[1], "debug")) {
        const char *rhs = find_eq_rhs(argv[1], (argc >= 3 ? argv[2] : NULL));
        if (!rhs) { log_printf("Usage: set debug=<hex|names>\n"); return -1; }

        debug_flags_t flags = parse_debug_expr(rhs);
        arm7vm_set_debug(cli->machine, flags);
        log_printf("[DEBUG] debug_flags set to 0x%08X\n", flags);
        return 0;
    }

    if (ieq(argv[1], "cpu")) {
        if (argc < 3) {
            log_printf("Usage: set cpu debug=<hex|names>\n");
            return -1;
        }

        if (ieq(argv[2], "debug") || istarts_with(argv[2], "debug=")) {
            const char *rhs = find_eq_rhs(argv[2], (argc >= 4 ? argv[3] : NULL));
            if (!rhs) {
                log_printf("Usage: set cpu debug=<hex|names>\n");
                return -1;
            }

            debug_flags_t flags = parse_debug_expr(rhs);
            arm7vm_set_debug(cli->machine, flags);
            log_printf("[DEBUG] debug_flags set to 0x%08X\n", flags);
            return 0;
        }

        log_printf("Unknown CPU setting. Try: set cpu debug=<...>\n");
        return -1;
    }

    if (ieq(argv[1], "mem")) {
        const char *rhs = find_eq_rhs(argv[1], (argc >= 3 ? argv[2] : NULL));
        if (!rhs) {
            log_printf("Usage: set mem <bytes|K|M|G>\n");
            return -1;
        }

        size_t bytes = 0;
        if (!parse_size_arg(rhs, &bytes)) {
            log_printf("set mem: invalid size '%s' (use e.g. 512M, 1G, 65536, 0x8000000)\n", rhs);
            return -1;
        }

        if (!arm7vm_add_ram(cli->machine, bytes)) {
            log_printf("Failed to set memory to %zu bytes. "
                       "If RAM is already attached, a replace-RAM API may be needed.\n",
                       bytes);
            return -1;
        }

        if (debug_flags & DBG_CLI)
            log_printf("Memory size set to %zu bytes\n", bytes);
        return 0;
    }

    if (ieq(argv[1], "trace")) {
        const char *rhs = find_eq_rhs(argv[1], (argc >= 3 ? argv[2] : NULL));
        if (!rhs) { log_printf("Usage: set trace=on|off\n"); return -1; }

        char buf[8];
        size_t n = 0;
        while (rhs[n] && n < sizeof(buf)-1 && !isspace((unsigned char)rhs[n])) {
            buf[n] = (char)tolower((unsigned char)rhs[n]);
            n++;
        }
        buf[n] = '\0';

        if (!strcmp(buf, "on") || !strcmp(buf, "1") || !strcmp(buf, "true") ||
            !strcmp(buf, "yes")) {
            trace_all = true;
            log_printf("[TRACE] trace_all enabled\n");
        } else if (!strcmp(buf, "off") || !strcmp(buf, "0") ||
                   !strcmp(buf, "false") || !strcmp(buf, "no")) {
            trace_all = false;
            log_printf("[TRACE] trace_all disabled\n");
        } else {
            log_printf("Usage: set trace=on|off\n");
            return -1;
        }
        return 0;
    }

    log_printf("Unknown setting. Supported:\n");
    log_printf("  set rN <value>            (N=0..15)\n");
    log_printf("  set pc <value>\n");
    log_printf("  set mem <bytes|K|M|G>\n");
    log_printf("  set debug=<hex|names>\n");
    log_printf("  set cpu debug=<hex|names>\n");
    log_printf("  set trace=on|off\n");
    log_printf("Names: INSTR, MEM_READ, MEM_WRITE, MMIO, DISK, IRQ, DISASM, K12, TRACE, CLI, ALL, NONE\n");
    return -1;
}
