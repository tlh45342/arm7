AS      := arm-none-eabi-as
LD      := arm-none-eabi-ld
OBJCOPY := arm-none-eabi-objcopy
PYTHON  := python

TARGET := test_simple_flat_load_monitor

.PHONY: all test clean

all: $(TARGET).bin

$(TARGET).o: $(TARGET).s
	$(AS) -o $@ $<

$(TARGET).elf: $(TARGET).o linker.ld
	$(LD) -T linker.ld -o $@ $(TARGET).o

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

test: $(TARGET).bin
	$(PYTHON) run_tests.py

clean:
	@if exist "$(TARGET).o" del /Q "$(TARGET).o"
	@if exist "$(TARGET).elf" del /Q "$(TARGET).elf"
	@if exist "$(TARGET).bin" del /Q "$(TARGET).bin"
	@if exist "disk0-test.img" del /Q "disk0-test.img"
	@if exist "build.script" del /Q "build.script"
	@if exist "DUMMY.BIN" del /Q "DUMMY.BIN"
	@if exist "BOOT.BIN" del /Q "BOOT.BIN"
	@if exist "MONITOR.BIN" del /Q "MONITOR.BIN"
