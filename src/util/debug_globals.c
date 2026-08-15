#include <stdbool.h>
#include <debug.h>

debug_flags_t debug_flags = DBG_NONE;  // default: no per-instruction spam
bool          trace_all   = false;     // default: off