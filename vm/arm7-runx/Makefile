# arm7-runx - Windows graphical ARM7 developer runner
#
# Layer 0 intentionally stays small:
#   - Win32 window + menus
#   - libarm7vm machine creation
#   - 80x25 CRT rendering
#   - flat binary load at 0x8000
#   - single-step / bounded 1000-instruction run
#   - disk0 attach
#
# This target is Windows-only by design.

PROJECT := arm7-runx
VERSION := 0.0.5

ROOT := ../..
LIBARM7VM_DIR := ../libarm7vm
LIBARM7VM := $(LIBARM7VM_DIR)/lib/libarm7vm.a

SRC_DIR := src
TOOLS_DIR := tools
BIOS_BIN := ../../firmware/bios/bios.bin
BIOS_HEADER := $(SRC_DIR)/default_bios.h
EMBED_BIOS := $(TOOLS_DIR)/embed_bios.py
PYTHON ?= python
BUILD_DIR := build
BIN_DIR := bin

ifeq ($(OS),Windows_NT)
	CC := gcc
	EXEEXT := .exe
else
$(error arm7-runx is intentionally Windows-only)
endif

CPPFLAGS := -I$(LIBARM7VM_DIR)/include
CFLAGS := -std=c11 -Wall -Wextra -O2
LDFLAGS := -mwindows
LDLIBS := -lcomdlg32 -lgdi32 -luser32

PREFIX ?= $(USERPROFILE)/.local
BINDIR ?= $(PREFIX)/bin

TARGET := $(BIN_DIR)/arm7-runx$(EXEEXT)
OBJECTS := $(BUILD_DIR)/main.o

MKDIR_P = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
RM_RF = if exist "$(subst /,\,$1)" rmdir /S /Q "$(subst /,\,$1)"
COPY_FILE = copy /Y "$(subst /,\,$1)" "$(subst /,\,$2)" >NUL

.PHONY: all clean rebuild install info buildbios

all: $(TARGET)

info:
	@echo Project: $(PROJECT)
	@echo Version: $(VERSION)
	@echo Platform: Windows only
	@echo Target: $(TARGET)
	@echo Core library: $(LIBARM7VM)

$(LIBARM7VM):
	$(MAKE) -C $(LIBARM7VM_DIR)

$(TARGET): $(LIBARM7VM) $(OBJECTS)
	@$(call MKDIR_P,$(BIN_DIR))
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBARM7VM) $(LDLIBS) -o $@
	@echo Built $(TARGET)

$(BIOS_BIN):
	$(MAKE) -C ../../firmware/bios

buildbios:
	$(MAKE) -C ../../firmware/bios
	$(PYTHON) $(EMBED_BIOS) $(BIOS_BIN) $(BIOS_HEADER)

$(BIOS_HEADER): $(BIOS_BIN) $(EMBED_BIOS)
	$(PYTHON) $(EMBED_BIOS) $(BIOS_BIN) $(BIOS_HEADER)

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(BIOS_HEADER)
	@$(call MKDIR_P,$(BUILD_DIR))
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	@$(call MKDIR_P,$(BINDIR))
	@$(call COPY_FILE,$(TARGET),$(BINDIR)/arm7-runx$(EXEEXT))
	@echo Installed arm7-runx to $(BINDIR)

clean:
	@$(call RM_RF,$(BUILD_DIR))
	@$(call RM_RF,$(BIN_DIR))

rebuild: clean all
