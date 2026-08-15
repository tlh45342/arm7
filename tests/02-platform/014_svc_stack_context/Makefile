AS:=arm-none-eabi-as
LD:=arm-none-eabi-ld
OBJCOPY:=arm-none-eabi-objcopy
PYTHON:=python
.PHONY: all test clean
all: svc_kernel.bin svc_user.bin
svc_kernel.o: svc_kernel.s
	$(AS) -o $@ $<
svc_kernel.elf: svc_kernel.o kernel.ld
	$(LD) -T kernel.ld -o $@ $<
svc_kernel.bin: svc_kernel.elf
	$(OBJCOPY) -O binary $< $@
svc_user.o: svc_user.s
	$(AS) -o $@ $<
svc_user.elf: svc_user.o user.ld
	$(LD) -T user.ld -o $@ $<
svc_user.bin: svc_user.elf
	$(OBJCOPY) -O binary $< $@
test: all
	$(PYTHON) run_tests.py
clean:
	@if exist "svc_kernel.o" del /Q "svc_kernel.o"
	@if exist "svc_kernel.elf" del /Q "svc_kernel.elf"
	@if exist "svc_kernel.bin" del /Q "svc_kernel.bin"
	@if exist "svc_user.o" del /Q "svc_user.o"
	@if exist "svc_user.elf" del /Q "svc_user.elf"
	@if exist "svc_user.bin" del /Q "svc_user.bin"
