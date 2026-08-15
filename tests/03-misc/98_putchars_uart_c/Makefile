# Makefile — bare-metal UART putchars test (single C file)

# Base name (no extension)
TARGET   := putchars_uart_c

SRC      := $(TARGET).c
OBJ      := $(TARGET).o
ELF      := $(TARGET).elf
BIN      := $(TARGET).bin
LST      := $(TARGET).lst

# Allow overriding tool prefix: e.g. make CROSS=arm-none-eabi-
CROSS    ?= arm-none-eabi-

CC       := $(CROSS)gcc
OBJCOPY  := $(CROSS)objcopy
OBJDUMP  := $(CROSS)objdump

# Toolchain flags
CFLAGS   := -std=c11 -Wall -Wextra -O2 \
            -ffreestanding -nostdlib \
            -march=armv7-a -marm

LDFLAGS  := -nostdlib -Wl,-Ttext=0x8000 -Wl,--build-id=none

# Explicit rm.exe (no wildcards for now to avoid glob issues)
RM_F     := "C:/Program Files/GNU/bin/rm.exe" -f

# ---------------------------------------------------------------------
# Build rules
# ---------------------------------------------------------------------

all: $(BIN)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(ELF): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(LST): $(ELF)
	$(OBJDUMP) -d $< > $@

# ---------------------------------------------------------------------
# Convenience targets
# ---------------------------------------------------------------------

code: $(SRC)
	$(CC) $(CFLAGS) -S -o $(TARGET).s $<

objdump: $(LST)

test: all
	python run_tests.py

clean:
	@echo "Cleaning build artifacts"
	-@$(RM_F) $(OBJ) $(ELF) $(BIN) $(LST) $(TARGET).s

.PHONY: all clean test code objdump
