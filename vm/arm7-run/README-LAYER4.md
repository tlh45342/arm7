# ARM7 Layer 4 — runner on public libarm7vm API

Layer 4 moves the ordinary `arm7-run` machine path off the legacy `VM *`
type and onto the opaque `arm7vm_machine_t *` API.

The runner now uses public library calls for:

- create / destroy
- load
- run / step
- register writes
- memory examination
- CRT display reads
- halt clearing
- register dump
- debug configuration
- disk0 attach
- UART initialization

The legacy CPU/HW/VM sources are still physically located under the root
`src/` tree and still compiled into `libarm7vm.a`.  That relocation is a
later layer.

## Test

From the repository root:

```text
make -C vm\libarm7vm clean
make -C vm\libarm7vm
make -C vm\libarm7vm test

make -C vm\arm7-run clean
make -C vm\arm7-run
make -C vm\arm7-run test
```

A useful manual sanity check is:

```text
vm\arm7-run\bin\arm7-run.exe
arm7-run> help
arm7-run> version
arm7-run> quit
```

Existing core instruction tests can still be used as the behavioral migration
gate after installing/copying `arm7-run.exe` into the expected test path.
