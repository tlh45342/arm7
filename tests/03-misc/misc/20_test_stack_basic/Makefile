AS      = arm-none-eabi-as
LD      = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

ASFLAGS = -mcpu=cortex-a15
LDFLAGS = -T linker.ld

SRC     = test_stack.s
OBJ     = test_stack.o
ELF     = test_stack.elf
BIN     = test_stack.bin

all: $(BIN)

$(OBJ): $(SRC)
	$(AS) $(ASFLAGS) $< -o $@

$(ELF): $(OBJ)
	$(LD) $(LDFLAGS) $^ -o $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f $(OBJ) $(ELF) $(BIN)
	
test:
	python run_tests.py