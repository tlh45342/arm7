AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

TARGET = test_sbc
ENTRY  = 0x8000

all: $(TARGET).bin objdump

$(TARGET).o: $(TARGET).s
	$(AS) -o $@ $<

$(TARGET).elf: $(TARGET).o linker.ld
	$(LD) -T linker.ld -o $@ $<

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

test: $(TARGET).bin
	python run_tests.py

clean:
	rm -f *.o *.elf *.bin *.log *.lst
	
objdump:
	arm-none-eabi-objdump -d $(TARGET).elf > $(TARGET).lst