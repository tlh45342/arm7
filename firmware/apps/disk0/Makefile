ROOT := O:\arm7

FLATDISK := flatdisk.exe

BOOT_BIN    := $(ROOT)\firmware\boot\boot.bin
MONITOR_BIN := $(ROOT)\firmware\monitor\monitor.bin

BUILD   := build
IMAGE   := $(BUILD)\disk.img
BOOT    := $(BUILD)\BOOT.BIN
MONITOR := $(BUILD)\MONITOR.BIN

.PHONY: all build disk list clean

all: build disk

build:
	$(MAKE) -C $(ROOT)\firmware\boot
	$(MAKE) -C $(ROOT)\firmware\monitor

	@if not exist "$(BUILD)" mkdir "$(BUILD)"

	copy /Y "$(BOOT_BIN)" "$(BOOT)"
	copy /Y "$(MONITOR_BIN)" "$(MONITOR)"

disk:
	$(FLATDISK) do build.script

list:
	$(FLATDISK) list "$(IMAGE)"

clean:
	@if exist "$(BUILD)" rmdir /S /Q "$(BUILD)"