# 129_test_b

Validates ARM `B` control flow using both unconditional and conditional forms.

Coverage:

- forward unconditional branch
- backward `BNE` loop
- taken `BEQ`
- not-taken `BEQ`
- final unconditional branch over a failure path

Guest results are stored at `0x00100000` so the test checks behavior rather
than relying only on disassembly text.

Run:

```text
make test
```
