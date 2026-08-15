# 006_simple_flat_load_boot

Third layer of the ARM7 SIMPLE-FLAT boot proof ladder.

This extends `005_simple_flat_directory` from "find BOOT.BIN" to "load the
complete BOOT.BIN payload into guest RAM."

The host uses the real `flatdisk` utility and creates a 700-byte BOOT.BIN.
The size is intentional: it requires two 512-byte disk reads.

The guest:

1. reads and validates the SIMPLE-FLAT header;
2. reads the directory and finds BOOT.BIN;
3. obtains start LBA, byte length, and flags;
4. calculates ceil(byte_length / 512);
5. loads all required sectors contiguously at `0x00010000`;
6. records independent result markers;
7. stops at BKPT.

The harness dumps the loaded guest RAM and compares the first 700 bytes
byte-for-byte with the original host BOOT.BIN.

Run:

    make -C tests\02-platform\006_simple_flat_load_boot test
