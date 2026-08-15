AS      = arm-none-eabi-as
LD      = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump

TARGET = bootrom

all: $(TARGET).bin objdump

$(TARGET).o: bootrom.s
	$(AS) -o $@ $<

$(TARGET).elf: $(TARGET).o linker.ld
	$(LD) -T linker.ld -o $@ $<

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

objdump: $(TARGET).elf
	$(OBJDUMP) -d $(TARGET).elf > $(TARGET).lst

clean:
	rm -f *.o *.elf *.bin *.lst *.log

run:
	arm-vm < bootrom.script