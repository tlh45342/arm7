// src/include/log.h
#pragma once
#include <stdbool.h>

bool log_set_file(const char* filename);
void start_log(const char* filename);
void stop_log(void);
bool log_is_active(void);

void log_printf(const char* fmt, ...);
void log_info  (const char* fmt, ...);
void log_warn  (const char* fmt, ...);
void log_error (const char* fmt, ...);
