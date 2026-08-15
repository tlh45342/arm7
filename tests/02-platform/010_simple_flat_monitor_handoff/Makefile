AS      := arm-none-eabi-as
LD      := arm-none-eabi-ld
OBJCOPY := arm-none-eabi-objcopy
PYTHON  := python

TARGET := test_simple_flat_monitor_handoff

.PHONY: all test clean

all: $(TARGET).bin MONITOR.BIN

$(TARGET).o: $(TARGET).s
	$(AS) -o $@ $<

$(TARGET).elf: $(TARGET).o linker.ld
	$(LD) -T linker.ld -o $@ $(TARGET).o

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

monitor-stub.o: monitor-stub.s
	$(AS) -o $@ $<

monitor-stub.elf: monitor-stub.o monitor-stub.ld
	$(LD) -T monitor-stub.ld -o $@ monitor-stub.o

MONITOR.BIN: monitor-stub.elf
	$(OBJCOPY) -O binary $< $@

test: all
	$(PYTHON) run_tests.py

clean:
	@if exist "$(TARGET).o" del /Q "$(TARGET).o"
	@if exist "$(TARGET).elf" del /Q "$(TARGET).elf"
	@if exist "$(TARGET).bin" del /Q "$(TARGET).bin"
	@if exist "monitor-stub.o" del /Q "monitor-stub.o"
	@if exist "monitor-stub.elf" del /Q "monitor-stub.elf"
	@if exist "MONITOR.BIN" del /Q "MONITOR.BIN"
	@if exist "DUMMY.BIN" del /Q "DUMMY.BIN"
	@if exist "BOOT.BIN" del /Q "BOOT.BIN"
	@if exist "disk0-test.img" del /Q "disk0-test.img"
	@if exist "build.script" del /Q "build.script"
