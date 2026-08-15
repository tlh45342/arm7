AS      = arm-none-eabi-as
LD      = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

ASFLAGS = -mcpu=cortex-a15
LDFLAGS = -T linker.ld

SRC     = test_stack_bllr.s
OBJ     = test_stack_bllr.o
ELF     = test_stack_bllr.elf
BIN     = test_stack_bllr.bin

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