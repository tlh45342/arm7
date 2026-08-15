# 50_test_disk_read

## Purpose

This test verifies that the ARM guest can communicate with the ARM-VM disk device and successfully read a sector from a disk image.

The goal is not to parse a filesystem. The goal is simply to prove that:

1. The guest can issue a disk read request.
2. ARM-VM can service the request.
3. Sector data is copied into guest memory.
4. The guest can access and use the returned data.

## Device Contract

The test uses the original ARM-VM disk MMIO interface:

| Register | Address        |
| -------- | -------------- |
| CMD      | DISK_BASE + 0  |
| LBA      | DISK_BASE + 4  |
| BUFFER   | DISK_BASE + 8  |
| STATUS   | DISK_BASE + 12 |

Where:

```
DISK_BASE = 0x0B000000
```

The guest writes:

- LBA number
- Destination buffer address
- Command = READ

ARM-VM copies one 512-byte sector into guest RAM.

## Test Procedure

The test reads:

```
LBA 19
```

into:

```
0x00010000
```

The guest then:

1. Reads the first byte from the returned buffer.
2. Writes the byte to UART.
3. Halts using the DEADBEEF sentinel.

## Expected Disk Image

Historically this test used a FAT12 floppy image containing:

```
HELLO.TXT
```

The expected beginning of sector 19 was:

```
48 45 4C 4C 4F 20 20 20
54 58 54 20
```

which corresponds to:

```
HELLO   TXT
```

This is a FAT directory entry, not the contents of the file.

## What This Test Proves

Successful execution demonstrates:

- MMIO register writes work.
- Disk device dispatch works.
- Sector reads work.
- Guest memory writes work.
- Guest memory reads work.
- UART output works.

This test serves as the foundation for later work involving:

- FAT12
- FAT32
- ext2
- BootROM disk loading
- Kernel loading
- Filesystem drivers

## Historical Notes

This was one of the earliest storage tests developed for ARM-VM and predates the BootROM console work.

The test intentionally avoids filesystem parsing and focuses only on raw sector access.