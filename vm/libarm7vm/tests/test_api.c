#include "arm7vm/arm7vm.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEST_RAM_SIZE (1024u * 1024u)
#define TEST_LOAD_ADDR 0x00008000u
#define TEST_FILE "arm7vm-api-test.bin"

static int pass_count = 0;
static int fail_count = 0;

static void check(int condition, const char *name)
{
    if (condition) {
        printf("  PASS %s\n", name);
        ++pass_count;
    } else {
        printf("  FAIL %s\n", name);
        ++fail_count;
    }
}

int main(void)
{
    static const uint8_t image[] = {
        0x11, 0x22, 0x33, 0x44,
        0xAA, 0xBB, 0xCC, 0xDD
    };
    uint8_t buffer[sizeof(image)];
    FILE *fp;
    arm7vm_machine_t *machine;

    printf("Running libarm7vm public API validation...\n");

    fp = fopen(TEST_FILE, "wb");
    check(fp != NULL, "test image created");
    if (!fp)
        return 1;

    check(
        fwrite(image, 1, sizeof(image), fp) == sizeof(image),
        "test image written"
    );
    fclose(fp);

    machine = arm7vm_create(TEST_RAM_SIZE);
    check(machine != NULL, "machine created through public API");

    if (machine) {
        check(
            arm7vm_load_binary(machine, TEST_FILE, TEST_LOAD_ADDR),
            "binary loaded through public API"
        );

        memset(buffer, 0, sizeof(buffer));
        check(
            arm7vm_read_memory(
                machine,
                TEST_LOAD_ADDR,
                buffer,
                sizeof(buffer)
            ),
            "memory read through public API"
        );

        check(
            memcmp(buffer, image, sizeof(image)) == 0,
            "loaded bytes match source image"
        );

        check(
            arm7vm_set_register(machine, 15u, TEST_LOAD_ADDR),
            "program counter set through register API"
        );

        check(
            !arm7vm_set_register(machine, 16u, 0),
            "invalid register rejected"
        );

        arm7vm_reset(machine);
        check(1, "machine reset through public API");

        arm7vm_clear_halt(machine);
        check(1, "halt state clear through public API");

        check(
            arm7vm_keyboard_pending(machine) == 0u,
            "keyboard FIFO initially empty through public API"
        );

        check(
            arm7vm_keyboard_push(machine, (uint8_t)'A'),
            "keyboard byte queued through public API"
        );

        check(
            arm7vm_keyboard_pending(machine) == 1u,
            "keyboard pending count visible through public API"
        );

        arm7vm_destroy(machine);
        check(1, "machine destroyed through public API");
    }

    remove(TEST_FILE);

    if (fail_count != 0) {
        printf(
            "libarm7vm public API: FAIL (%d passed, %d failed)\n",
            pass_count,
            fail_count
        );
        return 1;
    }

    printf("libarm7vm public API: PASS (%d/%d cases)\n",
           pass_count, pass_count);
    return 0;
}
