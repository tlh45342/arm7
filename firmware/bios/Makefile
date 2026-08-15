AS      := arm-none-eabi-as
LD      := arm-none-eabi-ld
OBJCOPY := arm-none-eabi-objcopy
OBJDUMP := arm-none-eabi-objdump

TARGET := bios
TEST_BOOT := tests/boot-stub

.PHONY: all test clean

all: $(TARGET).bin $(TARGET).lst

$(TARGET).o: $(TARGET).s
	$(AS) -o $@ $<

$(TARGET).elf: $(TARGET).o linker.ld
	$(LD) -T linker.ld -o $@ $(TARGET).o

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).lst: $(TARGET).elf
	$(OBJDUMP) -d $< > $@

$(TEST_BOOT).o: $(TEST_BOOT).s
	$(AS) -o $@ $<

$(TEST_BOOT).elf: $(TEST_BOOT).o tests/boot-stub.ld
	$(LD) -T tests/boot-stub.ld -o $@ $(TEST_BOOT).o

$(TEST_BOOT).bin: $(TEST_BOOT).elf
	$(OBJCOPY) -O binary $< $@

test: $(TARGET).bin $(TEST_BOOT).bin
	python tests/run_tests.py

clean:
	@rm -f $(wildcard *.o *.elf *.bin *.lst *.log)
	@rm -f $(wildcard tests/*.o tests/*.elf tests/*.bin tests/*.lst tests/*.log tests/*.img)
