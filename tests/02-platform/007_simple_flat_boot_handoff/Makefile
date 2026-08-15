AS      := arm-none-eabi-as
LD      := arm-none-eabi-ld
OBJCOPY := arm-none-eabi-objcopy
PYTHON  := python

TARGET := test_simple_flat_boot_handoff

.PHONY: all test clean

all: $(TARGET).bin BOOT.BIN

$(TARGET).o: $(TARGET).s
	$(AS) -o $@ $<

$(TARGET).elf: $(TARGET).o linker.ld
	$(LD) -T linker.ld -o $@ $(TARGET).o

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

boot-stub.o: boot-stub.s
	$(AS) -o $@ $<

boot-stub.elf: boot-stub.o boot-stub.ld
	$(LD) -T boot-stub.ld -o $@ boot-stub.o

BOOT.BIN: boot-stub.elf
	$(OBJCOPY) -O binary $< $@

test: all
	$(PYTHON) run_tests.py

clean:
	@if exist "$(TARGET).o" del /Q "$(TARGET).o"
	@if exist "$(TARGET).elf" del /Q "$(TARGET).elf"
	@if exist "$(TARGET).bin" del /Q "$(TARGET).bin"
	@if exist "boot-stub.o" del /Q "boot-stub.o"
	@if exist "boot-stub.elf" del /Q "boot-stub.elf"
	@if exist "BOOT.BIN" del /Q "BOOT.BIN"
	@if exist "DUMMY.BIN" del /Q "DUMMY.BIN"
	@if exist "disk0-test.img" del /Q "disk0-test.img"
	@if exist "build.script" del /Q "build.script"
