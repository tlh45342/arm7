AS      := arm-none-eabi-as
LD      := arm-none-eabi-ld
OBJCOPY := arm-none-eabi-objcopy
OBJDUMP := arm-none-eabi-objdump

TARGET := test_b

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

test: all
	python run_tests.py

clean:
	rm -f $(TARGET).o $(TARGET).elf $(TARGET).bin $(TARGET).lst $(TARGET).log
