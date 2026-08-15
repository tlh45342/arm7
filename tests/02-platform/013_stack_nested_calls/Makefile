AS      := arm-none-eabi-as
LD      := arm-none-eabi-ld
OBJCOPY := arm-none-eabi-objcopy
PYTHON  := python
TARGET  := test_stack_nested_calls

.PHONY: all test clean
all: $(TARGET).bin
$(TARGET).o: $(TARGET).s
	$(AS) -o $@ $<
$(TARGET).elf: $(TARGET).o linker.ld
	$(LD) -T linker.ld -o $@ $(TARGET).o
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
test: all
	$(PYTHON) run_tests.py
clean:
	@if exist "$(TARGET).o" del /Q "$(TARGET).o"
	@if exist "$(TARGET).elf" del /Q "$(TARGET).elf"
	@if exist "$(TARGET).bin" del /Q "$(TARGET).bin"
