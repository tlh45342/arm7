AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

SRC = test_ldrb.s
OBJ = test_ldrb.o
ELF = test_ldrb.elf
BIN = test_ldrb.bin
LDS = linker.ld
PYTHON_TEST = run_tests.py

all: $(BIN)

$(OBJ): $(SRC)
	$(AS) -mcpu=cortex-a15 -o $@ $<

$(ELF): $(OBJ) $(LDS)
	$(LD) -T $(LDS) -o $@ $(OBJ)

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o *.elf *.bin
	
test: $(TARGET)
	python $(PYTHON_TEST)
	
objdump:
	arm-none-eabi-objdump -d test_ldrb.elf