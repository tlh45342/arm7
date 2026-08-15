# 130_test_bl

Behavioral test for ARM `BL` (Branch with Link).

The test verifies:

- control transfers into the subroutine;
- `LR` receives the address immediately following the `BL`;
- `BX LR` returns to the caller;
- caller execution resumes normally.

The harness derives the expected LR value from `test_bl.lst` instead of
hard-coding the instruction address. This keeps the test stable if setup code
moves.

Run:

```text
make clean
make test
```
