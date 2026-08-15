# Makefile for barmental AS to C jump link test

TARGET      := test_cross
ELF         := $(TARGET).elf
BIN         := $(TARGET).bin
ASM         := start.s
SRC         := main.c
OBJS        := start.o main.o
LD_SCRIPT   := linker.ld
CC          := arm-none-eabi-gcc
AS          := arm-none-eabi-as
LD          := arm-none-eabi-ld
OBJCOPY     := arm-none-eabi-objcopy
NM          := arm-none-eabi-nm
OBJDUMP     := arm-none-eabi-objdump

CFLAGS      := -c -nostdlib -nostartfiles -ffreestanding -mcpu=cortex-a15 -Wall -Werror
LDFLAGS     := -T $(LD_SCRIPT)

.PHONY: all clean debug

all: $(BIN)

# Compile C source
main.o: $(SRC)
	$(CC) $(CFLAGS) $< -o $@

code:  $(SRC)
	$(CC) $(CFLAGS) -S -o main.o main.c

# Assemble startup
start.o: $(ASM)
	$(AS) $< -o $@

# Link everything
$(ELF): $(OBJS) $(LD_SCRIPT)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# Convert to raw binary
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# Clean build artifacts
clean:
	rm -f $(OBJS) $(ELF) $(BIN)

# Debug symbols
test:
	python run_tests.py