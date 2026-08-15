# arm7-runx 0.0.5

Windows graphical developer runner for ARM7.

## Firmware preload slots

- BIOS: `0x00000000`
- Flat binary: `0x00008000`
- BOOT.BIN: `0x00010000`
- Monitor: `0x00020000`

Selecting a file configures a slot. `Machine -> Start` resets the VM and
reloads every configured slot at its fixed address.

## Start At

`Machine -> Start At` selects the initial PC independently of preload order:

- BIOS: `0x00000000`
- Flat: `0x00008000`
- BOOT: `0x00010000`
- Monitor: `0x00020000`

This makes it possible to test BOOT or Monitor directly, or to start at BIOS
and observe the normal firmware handoff.

`Step One` prepares the configured machine first when necessary, so a selected
Start At target can be debugged one instruction at a time.

## Register view

The existing register dump remains available through `View -> Registers`.
A docked live register pane is intentionally not implemented by reaching into
private VM structures. It should be added after `libarm7vm` exposes a public
register-read/state-snapshot accessor.
