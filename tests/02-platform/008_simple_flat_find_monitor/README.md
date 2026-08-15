# 008_simple_flat_find_monitor

First test in the BOOT -> MONITOR half of the boot ladder.

The real `flatdisk` utility creates an image containing, in order:

1. DUMMY.BIN
2. BOOT.BIN
3. MONITOR.BIN

The ARM guest must read the SIMPLE-FLAT header, obtain the directory LBA,
scan the directory, and locate `MONITOR.BIN`. It records MONITOR start LBA,
byte length, and flags for comparison with flatdisk's own list output.

This is intentionally lookup-only. It does not load or execute MONITOR yet.

Run:

    make -C tests\02-platform\008_simple_flat_find_monitor test
