# =============================================================================
# ARM7 repository root Makefile
#
# Canonical build interface for the ARM7 VM, firmware, tests, and installation.
#
# Common targets:
#   make all
#   make vm
#   make firmware
#   make test
#   make validation
#   make install
#   make do
#   make clean
#
# Windows/cmd.exe friendly.  Component Makefiles remain responsible for their
# own platform-specific details.
# =============================================================================

.PHONY: \
	all vm firmware \
	libarm7vm arm7-run arm7-runx \
	bios boot monitor \
	test test-vm test-platform test-firmware test-isa validation \
	install install-vm \
	clean clean-vm clean-firmware clean-tests \
	do info

# -----------------------------------------------------------------------------
# Normal build
# -----------------------------------------------------------------------------

all: vm firmware

vm: libarm7vm arm7-run arm7-runx

firmware: bios boot monitor

libarm7vm:
	$(MAKE) -C vm/libarm7vm

arm7-run: libarm7vm
	$(MAKE) -C vm/arm7-run

arm7-runx: libarm7vm
	$(MAKE) -C vm/arm7-runx

bios:
	$(MAKE) -C firmware/bios

boot:
	$(MAKE) -C firmware/boot

monitor:
	$(MAKE) -C firmware/monitor

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------

test: test-vm test-platform test-firmware

test-vm:
	$(MAKE) -C vm/libarm7vm test
	$(MAKE) -C vm/arm7-run test

# Platform tests are intentionally explicit for now.  Add new device tests here
# as they become stable (RTC, disk, IRQ/timer, etc.).
test-platform:
	$(MAKE) -C tests/02-platform/001_keyboard_mmio test
	$(MAKE) -C tests/02-platform/002_rtc_mmio test

# BIOS owns the current automated firmware-chain validation.
test-firmware:
	$(MAKE) -C firmware/bios test

# The ISA inventory runner is the broad instruction regression sweep.
test-isa:
	python tests/01-core-iset/run_all_tests.py

# Slow/broad confidence run.
validation: test test-isa

# -----------------------------------------------------------------------------
# Installation
# -----------------------------------------------------------------------------

install: install-vm

install-vm: libarm7vm arm7-run arm7-runx
	$(MAKE) -C vm/libarm7vm install
	$(MAKE) -C vm/arm7-run install
	$(MAKE) -C vm/arm7-runx install

# -----------------------------------------------------------------------------
# Cleaning
# -----------------------------------------------------------------------------

clean: clean-vm clean-firmware clean-tests

clean-vm:
	$(MAKE) -C vm/libarm7vm clean
	$(MAKE) -C vm/arm7-run clean
	$(MAKE) -C vm/arm7-runx clean

clean-firmware:
	$(MAKE) -C firmware/bios clean
	$(MAKE) -C firmware/boot clean
	$(MAKE) -C firmware/monitor clean

clean-tests:
	-$(MAKE) -C tests/02-platform/001_keyboard_mmio clean
	-$(MAKE) -C tests/02-platform/002_rtc_mmio clean

# -----------------------------------------------------------------------------
# Development convenience target
#
# "make do" is the canonical rebuild/test/install workflow.
#
# Deliberately sequence this rather than expressing everything as prerequisites:
# we want a predictable development transcript and to stop on the first error.
# -----------------------------------------------------------------------------

do:
	$(MAKE) clean
	$(MAKE) libarm7vm
	$(MAKE) -C vm/libarm7vm test
	$(MAKE) firmware
	$(MAKE) arm7-run
	$(MAKE) -C vm/arm7-run test
	$(MAKE) -C tests/02-platform/001_keyboard_mmio test
	$(MAKE) -C tests/02-platform/002_rtc_mmio test
	$(MAKE) arm7-runx
	$(MAKE) install-vm
	@echo.
	@echo ============================================================
	@echo ARM7 development build complete.
	@echo ============================================================

# -----------------------------------------------------------------------------
# Information
# -----------------------------------------------------------------------------

info:
	@echo ARM7 root build targets:
	@echo   all            Build VM and firmware
	@echo   vm             Build libarm7vm, arm7-run, arm7-runx
	@echo   firmware       Build BIOS, BOOT, monitor
	@echo   test           Run normal VM/platform/firmware tests
	@echo   validation     Run normal tests plus ISA inventory
	@echo   install        Install VM library and runners
	@echo   clean          Clean VM, firmware, and platform tests
	@echo   do             Clean, build, test, and install development tree
