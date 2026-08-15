// mem.h
#ifndef MEM_H
#define MEM_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "log.h"

typedef struct VM VM;   // opaque VM from vm.h

void   mem_init(void);          
void   mem_bind(uint8_t *base, size_t size);
void   mem_unbind(void);
bool   mem_is_bound(void);
size_t mem_size(void);

uint8_t  mem_read8 (VM *vm, uint32_t addr);
uint16_t mem_read16(VM *vm, uint32_t addr);
uint32_t mem_read32(VM *vm, uint32_t addr);

void mem_write8 (VM *vm, uint32_t addr, uint8_t  value);
void mem_write16(VM *vm, uint32_t addr, uint16_t value);
void mem_write32(VM *vm, uint32_t addr, uint32_t value);

bool     mem_copy_in (uint32_t dst_addr, const void *src, size_t len);
bool     mem_copy_out(void *dst, uint32_t src_addr, size_t len);

#endif
