AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

TARGET = test_ldrd
ENTRY  = 0x8000

all: $(TARGET).bin

$(TARGET).o: $(TARGET).s
	$(AS) -o $@ $<

$(TARGET).elf: $(TARGET).o linker.ld
	$(LD) -T linker.ld -o $@ $<

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

test:
	python run_tests.py

clean:
	rm -f *.o *.elf *.bin

objdump:
	arm-none-eabi-objdump -d $(TARGET).elf