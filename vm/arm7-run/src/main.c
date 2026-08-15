#include <stdio.h>

#include "arm7vm/arm7vm.h"
#include "default_bios.h"
#include "cli.h"

#define ARM7_RUN_DEFAULT_RAM (512u * 1024u * 1024u)
#define ARM7_RUN_UART_BASE   0x09000000u

int main(int argc, char **argv)
{
    arm7vm_machine_t *machine;
    CLI cli;

    (void)argc;
    (void)argv;

    machine = arm7vm_create(ARM7_RUN_DEFAULT_RAM);
    if (!machine) {
        fprintf(stderr, "arm7-run: failed to create machine\n");
        return 1;
    }

    /*
     * Standard ARM7 machine firmware.
     *
     * arm7-run starts with the generated built-in BIOS already present at
     * physical address 0x00000000.  The CLI remains fully authoritative:
     *
     *     load alternate-bios.bin 0x00000000
     *
     * simply overwrites these bytes and therefore acts as the BIOS override.
     */
    if (!arm7vm_load_buffer(
            machine,
            default_bios,
            default_bios_size,
            0x00000000u)) {
        fprintf(stderr,
                "arm7-run: failed to load built-in BIOS (%u bytes)\n",
                (unsigned)default_bios_size);
        arm7vm_destroy(machine);
        return 1;
    }

    arm7vm_uart_init(ARM7_RUN_UART_BASE);

    cli_init(&cli, machine, stdin, true);
    cli_run(&cli);

    arm7vm_destroy(machine);
    return 0;
}
