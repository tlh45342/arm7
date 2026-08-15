#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "log.h"
#include "debug.h"   // for debug_flags_t / DBG_* (optional but nice)

static FILE* log_file = NULL;

void start_log(const char* filename) {
    if (log_file) fclose(log_file);
    log_file = fopen(filename, "w");
    if (!log_file) {
        perror("Failed to open log file");
    }
}

void stop_log(void) {
    if (log_file) { fclose(log_file); log_file = NULL; }
}

bool log_set_file(const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return false;
    if (log_file) fclose(log_file);
    log_file = f;
    return true;
}

void log_printf(const char* fmt, ...) {
    va_list args; 
    va_start(args, fmt);

    if (log_file) {
        va_list copy; 
        va_copy(copy, args);
        vfprintf(log_file, fmt, copy);
        fflush(log_file);
        va_end(copy);
    }

    vprintf(fmt, args);
    va_end(args);
}

bool log_is_active(void) {
    return (log_file != NULL);
}

// Internal helper for prefixed logging
static void log_with_prefix(const char* prefix, const char* fmt, va_list ap) {
    if (log_file) {
        va_list copy1;
        va_copy(copy1, ap);
        fprintf(log_file, "%s", prefix);
        vfprintf(log_file, fmt, copy1);
        fflush(log_file);
        va_end(copy1);
    }

    va_list copy2;
    va_copy(copy2, ap);
    fprintf(stdout, "%s", prefix);
    vfprintf(stdout, fmt, copy2);
    va_end(copy2);
}

void log_info(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    log_with_prefix("[INFO] ", fmt, ap);
    va_end(ap);
}

void log_warn(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    log_with_prefix("[WARN] ", fmt, ap);
    va_end(ap);
}

void log_error(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    log_with_prefix("[ERROR] ", fmt, ap);
    va_end(ap);
}
