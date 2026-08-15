# 151_test_nop

Behavioral test for ARM `NOP`.

It writes one marker before the NOP and one marker after it, proving execution
continues across the instruction without altering the intended program flow.

Run:

```text
make clean
make test
```
