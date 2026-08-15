# 009_simple_flat_load_monitor

Second test in the BOOT -> MONITOR half of the boot ladder.

This extends `008_simple_flat_find_monitor` from lookup to full payload load.

The host creates a 1148-byte MONITOR.BIN, intentionally requiring three
512-byte disk reads. The guest finds the file through the SIMPLE-FLAT
directory, calculates the sector count, and loads the complete payload at:

    0x00020000

The harness dumps the loaded guest RAM and compares all 1148 bytes against
the original host MONITOR.BIN.

Run:

    make -C tests\02-platform\009_simple_flat_load_monitor test
