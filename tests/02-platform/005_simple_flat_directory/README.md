# 005_simple_flat_directory

Second layer of the ARM7 disk-boot proof ladder.

`004_disk_sector_read` uses a synthetic raw sector to prove disk transport.
This test intentionally switches to the real `flatdisk` utility so the two
independent sides of the SIMPLE-FLAT contract meet:

    flatdisk -> writes image format
    ARM guest -> reads and parses image format

The host creates a 1 MiB SIMPLE-FLAT image and installs:

1. `DUMMY.BIN`
2. `BOOT.BIN`

Putting DUMMY first forces the guest to scan the directory instead of simply
assuming BOOT is entry zero.

The guest then:

1. reads LBA 0 through the proven disk MMIO primitive;
2. verifies `SFLT`;
3. parses sector size, directory LBA, entry count, and entry size;
4. reads the directory sector using the LBA obtained from the header;
5. scans 64-byte entries for `BOOT.BIN`;
6. records BOOT start LBA, byte length, and flags;
7. reaches BKPT.

The Python harness compares the guest's discovered values with flatdisk's
own `list` output.

Run:

    make -C tests\02-platform\005_simple_flat_directory test
