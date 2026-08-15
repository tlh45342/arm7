arm7-runx 0.0.7 - Embedded BIOS selector

Firmware menu:
    [x] Embedded BIOS
        Load BIOS at 0x00000000...
        Load BOOT.BIN at 0x00010000...
        Load Monitor at 0x00020000...

Rules:
- Embedded BIOS is selected and checked at startup.
- Start At -> BIOS is valid immediately.
- Load BIOS... selects an external BIOS and clears the Embedded BIOS check.
- Choosing Embedded BIOS switches back to the compiled-in BIOS.
- Cancelling Load BIOS... leaves the current BIOS source unchanged.
- BOOT/Monitor developer preload behavior is unchanged.
- Keyboard and double-buffered CRT behavior are preserved.

Build:
    make -C vm\arm7-runx clean
    make -C vm\arm7-runx install

Test 1:
    Launch arm7-runx.
    Firmware -> Embedded BIOS should be checked.
    Leave external BIOS unselected.
    Load BOOT and Monitor as before.
    Start At -> BIOS.
    Start.

Test 2:
    Firmware -> Load BIOS...
    Choose firmware\bios\bios.bin.
    Embedded BIOS should become unchecked.
    Start should use the selected external BIOS.

Test 3:
    Firmware -> Embedded BIOS.
    Checkmark should return.
    Start should again use the compiled-in BIOS.
