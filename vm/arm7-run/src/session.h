/* session.h - user session context for CLI/REPL */

/*
 * Copyright 2025-2026 Thomas L Hamilton
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* Forward declarations (you can replace with real headers later) */
typedef struct VM VM;
typedef struct VMManager VMManager;

/* ---------------------------
 * Variables (simple key/value)
 * --------------------------- */
typedef struct sess_kv {
    char *key;
    char *val;
} sess_kv_t;

typedef struct sess_vars {
    sess_kv_t *items;
    size_t    count;
    size_t    cap;
} sess_vars_t;

/* ---------------------------
 * Output/log policy
 * --------------------------- */
typedef struct sess_out {
    FILE *log_fp;         /* NULL if disabled */
    bool  tee;            /* if true: print to console AND log */
    bool  quiet;          /* if true: suppress console output (log-only) */
} sess_out_t;

/* ---------------------------
 * Session
 * --------------------------- */
typedef struct Session {
    /* Output and logging */
    sess_out_t out;

    /* Shell/session variables (e.g., VMID=1) */
    sess_vars_t vars;

    /* VM selection context */
    int   current_vmid;       /* -1 if none selected */
    char *current_vmname;     /* optional; NULL if unused */

    /* VM control (optional: wire up later) */
    VMManager *vmm;           /* owned or borrowed; your choice */
    VM        *vm;            /* convenience: current VM pointer (optional) */

    /* Misc user-context knobs */
    unsigned debug_flags;     /* CLI-level debug flags (not CPU flags) */
    int      last_status;     /* last command status code */
} Session;

/* Lifecycle */
void session_init(Session *s);
void session_shutdown(Session *s);

/* Output helpers (console/log tee) */
void session_printf(Session *s, const char *fmt, ...);
void session_eprintf(Session *s, const char *fmt, ...);

/* Log control (simple) */
bool session_log_open(Session *s, const char *path, bool tee);
void session_log_close(Session *s);

/* Variable helpers */
const char *session_get(Session *s, const char *key);
bool        session_set(Session *s, const char *key, const char *val);
bool        session_unset(Session *s, const char *key);
void        session_vars_dump(Session *s);

