// src/include/hw_bus.h
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "vm.h"

// Use the same handler typedefs you expose from vm.h.
// If vm.h already defines these, you can omit these lines.
#ifndef VM_MMIO_FNS_DEFINED
typedef uint32_t (*vm_mmio_read_fn)(void *ctx, uint32_t offset);
typedef void     (*vm_mmio_write_fn)(void *ctx, uint32_t offset, uint32_t value);
#define VM_MMIO_FNS_DEFINED 1
#endif