// src/dtb_blob.c
#include <stddef.h>
#include "dtb_blob.h"

// If you already have a real FDT blob generator, replace these with that data.
// For now, an empty blob keeps the linker happy and vm_place_dtb() early-returns.
const unsigned char dtb_blob[] = { /* empty */ };
const size_t dtb_blob_len = 0;