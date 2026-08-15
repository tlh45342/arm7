AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump

SRC = test_disk_read.s
OBJ = test_disk_read.o
ELF = test_disk_read.elf
BIN = test_disk_read.bin
LDS = linker.ld
PY = run_tests.py

all: $(BIN)

$(OBJ): $(SRC)
	$(AS) -mcpu=cortex-a15 -o $@ $<

$(ELF): $(OBJ) $(LDS)
	$(LD) -T $(LDS) -o $@ $(OBJ)

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o *.elf *.bin
	
test:
	python $(PY)

objdump:
	$(OBJDUMP) -d test_disk_read.elf
