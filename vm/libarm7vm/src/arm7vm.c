#include "arm7vm/arm7vm.h"

#include "dev_disk.h"
#include "dev_uart.h"
#include "vm.h"

#include <stdlib.h>

struct arm7vm_machine {
    VM *legacy_vm;
};

arm7vm_machine_t *arm7vm_create(size_t ram_bytes)
{
    arm7vm_machine_t *machine;

    if (ram_bytes == 0)
        return NULL;

    machine = (arm7vm_machine_t *)calloc(1, sizeof(*machine));
    if (!machine)
        return NULL;

    machine->legacy_vm = vm_create();
    if (!machine->legacy_vm) {
        free(machine);
        return NULL;
    }

    vm_reset(machine->legacy_vm);

    /*
     * Establish the MMIO device bases before guest execution.
     * Keyboard injection can lazily initialize itself, but devices such
     * as the RTC depend on this initialization.
     */
    vm_devices_init(machine->legacy_vm);

    if (!vm_add_ram(machine->legacy_vm, ram_bytes)) {
        vm_destroy(machine->legacy_vm);
        free(machine);
        return NULL;
    }

    return machine;
}

void arm7vm_destroy(arm7vm_machine_t *machine)
{
    if (!machine)
        return;

    if (machine->legacy_vm)
        vm_destroy(machine->legacy_vm);

    machine->legacy_vm = NULL;
    free(machine);
}

void arm7vm_reset(arm7vm_machine_t *machine)
{
    if (machine && machine->legacy_vm)
        vm_reset(machine->legacy_vm);
}

bool arm7vm_load_binary(
    arm7vm_machine_t *machine,
    const char *path,
    uint32_t address
)
{
    if (!machine || !machine->legacy_vm || !path)
        return false;

    return vm_load_binary(machine->legacy_vm, path, address);
}

bool arm7vm_load_buffer(
    arm7vm_machine_t *machine,
    const void *buffer,
    size_t length,
    uint32_t address
)
{
    const uint8_t *bytes = (const uint8_t *)buffer;
    size_t i;

    if (!machine || !machine->legacy_vm || (!buffer && length != 0u))
        return false;

    for (i = 0; i < length; ++i) {
        if (!vm_write_mem8(machine->legacy_vm,
                           address + (uint32_t)i,
                           bytes[i]))
            return false;
    }

    return true;
}

bool arm7vm_set_register(
    arm7vm_machine_t *machine,
    unsigned register_number,
    uint32_t value
)
{
    if (!machine || !machine->legacy_vm || register_number > 15u)
        return false;

    vm_set_reg(machine->legacy_vm, (int)register_number, value);
    return true;
}

bool arm7vm_step(arm7vm_machine_t *machine)
{
    if (!machine || !machine->legacy_vm)
        return false;

    return vm_step(machine->legacy_vm);
}

void arm7vm_run(arm7vm_machine_t *machine, uint64_t max_steps)
{
    if (!machine || !machine->legacy_vm)
        return;

    vm_run(machine->legacy_vm, max_steps);
}

bool arm7vm_read_memory(
    arm7vm_machine_t *machine,
    uint32_t address,
    void *buffer,
    size_t length
)
{
    if (!machine || !machine->legacy_vm || !buffer)
        return false;

    return vm_read_mem(machine->legacy_vm, address, buffer, length);
}

void arm7vm_clear_halt(arm7vm_machine_t *machine)
{
    if (machine && machine->legacy_vm)
        vm_clear_halt(machine->legacy_vm);
}

bool arm7vm_keyboard_push(
    arm7vm_machine_t *machine,
    uint8_t ch
)
{
    if (!machine || !machine->legacy_vm)
        return false;

    return vm_keyboard_push(machine->legacy_vm, ch);
}

unsigned arm7vm_keyboard_pending(
    const arm7vm_machine_t *machine
)
{
    if (!machine || !machine->legacy_vm)
        return 0u;

    return vm_keyboard_pending(machine->legacy_vm);
}

void arm7vm_dump_registers(arm7vm_machine_t *machine)
{
    if (machine && machine->legacy_vm)
        vm_dump_regs(machine->legacy_vm);
}

void arm7vm_set_debug(arm7vm_machine_t *machine, uint32_t flags)
{
    if (machine && machine->legacy_vm)
        vm_set_debug(machine->legacy_vm, flags);
}

bool arm7vm_add_ram(arm7vm_machine_t *machine, size_t ram_bytes)
{
    if (!machine || !machine->legacy_vm || ram_bytes == 0)
        return false;

    return vm_add_ram(machine->legacy_vm, ram_bytes);
}

bool arm7vm_attach_disk0(arm7vm_machine_t *machine, const char *path)
{
    if (!machine || !machine->legacy_vm || !path)
        return false;

    return dev_disk0_attach(machine->legacy_vm, path);
}

void arm7vm_uart_init(uint32_t base_address)
{
    dev_uart_init(base_address);
}
