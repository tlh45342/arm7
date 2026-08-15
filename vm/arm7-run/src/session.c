/* src/session.c - minimal session implementation */

/*
 * Copyright 2026 Thomas L Hamilton
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

#include "session.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---------- small utils ---------- */

static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

static int kv_find(sess_vars_t *v, const char *key) {
    if (!v || !key) return -1;
    for (size_t i = 0; i < v->count; i++) {
        if (v->items[i].key && strcmp(v->items[i].key, key) == 0)
            return (int)i;
    }
    return -1;
}

static bool kv_reserve(sess_vars_t *v, size_t want_cap) {
    if (v->cap >= want_cap) return true;
    size_t new_cap = v->cap ? v->cap * 2 : 16;
    if (new_cap < want_cap) new_cap = want_cap;

    sess_kv_t *p = (sess_kv_t *)realloc(v->items, new_cap * sizeof(sess_kv_t));
    if (!p) return false;

    /* zero new slots */
    for (size_t i = v->cap; i < new_cap; i++) {
        p[i].key = NULL;
        p[i].val = NULL;
    }
    v->items = p;
    v->cap   = new_cap;
    return true;
}

/* ---------- lifecycle ---------- */

void session_init(Session *s) {
    if (!s) return;
    memset(s, 0, sizeof(*s));

    s->out.log_fp = NULL;
    s->out.tee    = true;
    s->out.quiet  = false;

    s->vars.items = NULL;
    s->vars.count = 0;
    s->vars.cap   = 0;

    s->current_vmid   = -1;
    s->current_vmname = NULL;

    s->vmm = NULL;
    s->vm  = NULL;

    s->debug_flags = 0;
    s->last_status = 0;
}

void session_shutdown(Session *s) {
    if (!s) return;

    session_log_close(s);

    /* free variables */
    for (size_t i = 0; i < s->vars.count; i++) {
        free(s->vars.items[i].key);
        free(s->vars.items[i].val);
        s->vars.items[i].key = NULL;
        s->vars.items[i].val = NULL;
    }
    free(s->vars.items);
    s->vars.items = NULL;
    s->vars.count = 0;
    s->vars.cap   = 0;

    free(s->current_vmname);
    s->current_vmname = NULL;

    /* Note: s->vmm and s->vm are not freed here by default.
       Wire ownership in later when you decide who owns the VMManager/VM. */
}

/* ---------- output ---------- */

static void session_vout(Session *s, FILE *console, const char *fmt, va_list ap_in) {
    if (!s) return;

    /* We need two passes if we might write to both console and log */
    va_list ap1, ap2;
    va_copy(ap1, ap_in);
    va_copy(ap2, ap_in);

    const bool have_log = (s->out.log_fp != NULL);
    const bool do_console = (!s->out.quiet);

    if (do_console) {
        vfprintf(console, fmt, ap1);
        fflush(console);
    }

    if (have_log) {
        if (s->out.tee || !do_console) {
            vfprintf(s->out.log_fp, fmt, ap2);
            fflush(s->out.log_fp);
        }
    }

    va_end(ap1);
    va_end(ap2);
}

void session_printf(Session *s, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    session_vout(s, stdout, fmt, ap);
    va_end(ap);
}

void session_eprintf(Session *s, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    session_vout(s, stderr, fmt, ap);
    va_end(ap);
}

/* ---------- log control ---------- */

bool session_log_open(Session *s, const char *path, bool tee) {
    if (!s || !path || !*path) return false;

    session_log_close(s);

    FILE *fp = fopen(path, "wb");
    if (!fp) return false;

    s->out.log_fp = fp;
    s->out.tee    = tee;
    return true;
}

void session_log_close(Session *s) {
    if (!s) return;
    if (s->out.log_fp) {
        fclose(s->out.log_fp);
        s->out.log_fp = NULL;
    }
}

/* ---------- variables ---------- */

const char *session_get(Session *s, const char *key) {
    if (!s || !key) return NULL;
    int idx = kv_find(&s->vars, key);
    if (idx < 0) return NULL;
    return s->vars.items[(size_t)idx].val;
}

bool session_set(Session *s, const char *key, const char *val) {
    if (!s || !key || !*key) return false;
    if (!val) val = "";

    int idx = kv_find(&s->vars, key);
    if (idx >= 0) {
        char *nv = xstrdup(val);
        if (!nv) return false;
        free(s->vars.items[(size_t)idx].val);
        s->vars.items[(size_t)idx].val = nv;
        return true;
    }

    if (!kv_reserve(&s->vars, s->vars.count + 1)) return false;

    s->vars.items[s->vars.count].key = xstrdup(key);
    s->vars.items[s->vars.count].val = xstrdup(val);
    if (!s->vars.items[s->vars.count].key || !s->vars.items[s->vars.count].val) {
        free(s->vars.items[s->vars.count].key);
        free(s->vars.items[s->vars.count].val);
        s->vars.items[s->vars.count].key = NULL;
        s->vars.items[s->vars.count].val = NULL;
        return false;
    }

    s->vars.count++;
    return true;
}

bool session_unset(Session *s, const char *key) {
    if (!s || !key) return false;
    int idx = kv_find(&s->vars, key);
    if (idx < 0) return false;

    size_t i = (size_t)idx;
    free(s->vars.items[i].key);
    free(s->vars.items[i].val);

    /* move last into this slot */
    if (i != s->vars.count - 1) {
        s->vars.items[i] = s->vars.items[s->vars.count - 1];
        s->vars.items[s->vars.count - 1].key = NULL;
        s->vars.items[s->vars.count - 1].val = NULL;
    }
    s->vars.count--;
    return true;
}

void session_vars_dump(Session *s) {
    if (!s) return;
    for (size_t i = 0; i < s->vars.count; i++) {
        const char *k = s->vars.items[i].key ? s->vars.items[i].key : "";
        const char *v = s->vars.items[i].val ? s->vars.items[i].val : "";
        session_printf(s, "%s=%s\n", k, v);
    }
}