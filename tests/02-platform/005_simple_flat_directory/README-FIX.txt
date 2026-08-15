005 harness-only fix

The ARM guest already succeeded. The old Python parser captured only the
first 16-byte row of the result block at 0x00030800.

This version collects consecutive arm7-run dump rows by address, so fields
at +0x10 and +0x20 are visible to the assertions.

No guest assembly, disk format, or VM code changes are included.

Retest:
    make -C tests\02-platform\005_simple_flat_directory test
