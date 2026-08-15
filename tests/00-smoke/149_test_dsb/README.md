# 149_test_dsb

Behavioral smoke test for ARM `DSB`.

The VM does not currently model a cache hierarchy or out-of-order memory
system, so this test intentionally does not claim to validate real hardware
ordering semantics. It proves the instruction is accepted and that execution
continues correctly across it.

Run:

```text
make clean
make test
```
