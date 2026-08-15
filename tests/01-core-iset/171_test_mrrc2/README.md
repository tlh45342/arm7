# 171_test_mrrc2

Dedicated diagnostic test for A32 `MRRC2`.

This directory replaces an inherited stale checker that was testing unrelated
MRS/CLZ/EOR output and used the misspelled `test_mmcr2` filename.

The current milestone is deliberately modest and honest:

- assemble and reach `MRRC2`;
- require an ARM7 decoder/dispatcher route for it;
- reject the VM's generic unknown-instruction/NOP fallback as success;
- verify control returns from an MRRC2 handler;
- reach BKPT.

The test does **not yet assert a specific CP15 64-bit return value**. That should
be added when the ARM7 VM has an explicit coprocessor model/contract.

Run:

```text
make clean
make test
```

A failure saying that MRRC2 is unknown is a useful result: it means the test
harness is repaired and the remaining work is CPU decode/implementation.
