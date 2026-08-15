# Makefile — 100_test_disk_read2 (single C file, no linker script)

TARGET   := test_disk_read2
SRC      := $(TARGET).c
ELF      := $(TARGET).elf
BIN      := $(TARGET).bin
MAP      := $(TARGET).map
LST      := $(TARGET).lst

CROSS    ?= arm-none-eabi-
CC       := $(CROSS)gcc
OBJCOPY  := $(CROSS)objcopy
OBJDUMP  := $(CROSS)objdump

CFLAGS   := -nostdlib -nostartfiles -ffreestanding -fno-builtin \
            -mcpu=cortex-a15 -O2 -Wall -Wextra -Werror
LDFLAGS  := -Wl,-Ttext=0x8000 -Wl,-e,_start -Wl,-Map=$(MAP)

.PHONY: all clean disasm hexdump

all: $(BIN)

$(ELF): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

disasm: $(ELF)
	$(OBJDUMP) -d -S $< > $(LST)

hexdump: $(BIN)
	hexdump $(BIN)

clean:
	rm -f $(ELF) $(BIN) $(MAP) $(LST) *.o *.log *.sym *.s
	
objdump:
	$(OBJDUMP) -d $(ELF)