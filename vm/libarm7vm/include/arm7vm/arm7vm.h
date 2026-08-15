#ifndef ARM7VM_ARM7VM_H
#define ARM7VM_ARM7VM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct arm7vm_machine arm7vm_machine_t;

arm7vm_machine_t *arm7vm_create(size_t ram_bytes);
void arm7vm_destroy(arm7vm_machine_t *machine);

void arm7vm_reset(arm7vm_machine_t *machine);

bool arm7vm_load_binary(
    arm7vm_machine_t *machine,
    const char *path,
    uint32_t address
);

bool arm7vm_load_buffer(
    arm7vm_machine_t *machine,
    const void *buffer,
    size_t length,
    uint32_t address
);

bool arm7vm_set_register(
    arm7vm_machine_t *machine,
    unsigned register_number,
    uint32_t value
);

bool arm7vm_step(arm7vm_machine_t *machine);
void arm7vm_run(arm7vm_machine_t *machine, uint64_t max_steps);

bool arm7vm_read_memory(
    arm7vm_machine_t *machine,
    uint32_t address,
    void *buffer,
    size_t length
);

void arm7vm_clear_halt(arm7vm_machine_t *machine);

/*
 * Host -> guest keyboard FIFO.
 *
 * Guest MMIO:
 *   0x0A001000 KEY_STATUS bit0 READY
 *   0x0A001004 KEY_DATA   low byte, read pops FIFO
 */
bool arm7vm_keyboard_push(
    arm7vm_machine_t *machine,
    uint8_t ch
);

unsigned arm7vm_keyboard_pending(
    const arm7vm_machine_t *machine
);

/*
 * Transitional runner-facing API.
 *
 * These calls preserve existing arm7-run behavior while keeping the
 * legacy VM type private to libarm7vm.
 */
void arm7vm_dump_registers(arm7vm_machine_t *machine);
void arm7vm_set_debug(arm7vm_machine_t *machine, uint32_t flags);
bool arm7vm_add_ram(arm7vm_machine_t *machine, size_t ram_bytes);
bool arm7vm_attach_disk0(arm7vm_machine_t *machine, const char *path);
void arm7vm_uart_init(uint32_t base_address);

#ifdef __cplusplus
}
#endif

#endif
