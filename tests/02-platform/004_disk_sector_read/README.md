# 004_disk_sector_read

Small automated guest diagnostic for the ARM7 disk boot path.

This intentionally does **not** involve BIOS, BOOT, MONITOR, or the
SIMPLE-FLAT directory parser. It isolates the one primitive BIOS needs first:

1. attach disk0 through `arm7-run`;
2. issue a disk READ for LBA 0;
3. wait for DRQ / reject ERR;
4. copy the full 512-byte MMIO DATA window into guest RAM;
5. verify the copied sector begins with `SFLT`;
6. terminate with BKPT.

The assembly sequence is intentionally BIOS-like. If this test passes while
the BIOS test fails, the disk device/MMIO primitive is good and the remaining
fault is in BIOS control flow or parsing.

Run from the repository root:

    make -C tests\02-platform\004_disk_sector_read test
