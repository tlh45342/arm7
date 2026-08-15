# arm7-run - headless ARM7 VM runner
#
# Layer 2 migration:
#   arm7-run now owns its CLI sources under vm/arm7-run/src.
#   The VM/CPU/HW implementation remains behind libarm7vm.a.
#
# No intentional runtime behavior changes in this layer.

PROJECT := arm7-run

ROOT       := ../..
LEGACY_SRC := $(ROOT)/src
SRC_DIR    := src
BUILD_DIR  := build
BIN_DIR    := bin
TEST_DIR   := tests
TOOLS_DIR  := tools

BIOS_BIN    := ../../firmware/bios/bios.bin
BIOS_HEADER := $(SRC_DIR)/default_bios.h
EMBED_BIOS  := $(TOOLS_DIR)/embed_bios.py

LIBARM7VM_DIR := ../libarm7vm
LIBARM7VM := $(LIBARM7VM_DIR)/lib/libarm7vm.a

ifeq ($(OS),Windows_NT)
	CC := gcc
	EXEEXT := .exe

	PREFIX ?= $(USERPROFILE)/.local
	BINDIR ?= $(PREFIX)/bin

	MKDIR_P = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
	RM_RF = if exist "$(subst /,\,$1)" rmdir /S /Q "$(subst /,\,$1)"
	COPY_FILE = copy /Y "$(subst /,\,$1)" "$(subst /,\,$2)" >NUL
else
	CC ?= cc
	EXEEXT :=

	PREFIX ?= $(HOME)/.local
	BINDIR ?= $(PREFIX)/bin

	MKDIR_P = mkdir -p "$1"
	RM_RF = rm -rf "$1"
	COPY_FILE = cp -f "$1" "$2"
endif

# Runner-local headers come first. Legacy include paths remain temporarily
# because cli.c/main.c/session.c still use the existing VM public/internal
# headers while libarm7vm is being extracted.
CPPFLAGS ?= -I$(SRC_DIR) -I$(LIBARM7VM_DIR)/include -I$(LEGACY_SRC)/include -I$(LEGACY_SRC)
CFLAGS ?= -std=c11 -Wall -Wextra -O2
PYTHON ?= python

RUN_SRC := $(wildcard $(SRC_DIR)/*.c)
RUN_OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(RUN_SRC))

ARM7_RUN := $(BIN_DIR)/arm7-run$(EXEEXT)

.PHONY: all test install clean rebuild info buildbios

all: $(ARM7_RUN)

info:
	@echo Project: $(PROJECT)
	@echo Runner source root: $(SRC_DIR)
	@echo Runner sources: $(words $(RUN_SRC))
	@echo Runner: $(ARM7_RUN)
	@echo Core library: $(LIBARM7VM)

LIBARM7VM_INPUTS := \
	$(wildcard $(LEGACY_SRC)/cpu/*.c) \
	$(wildcard $(LEGACY_SRC)/hw/*.c) \
	$(wildcard $(LEGACY_SRC)/vm/*.c) \
	$(wildcard $(LEGACY_SRC)/util/*.c) \
	$(wildcard $(LEGACY_SRC)/include/*.h)

$(LIBARM7VM): $(LIBARM7VM_INPUTS)
	$(MAKE) -C $(LIBARM7VM_DIR)

$(ARM7_RUN): $(LIBARM7VM) $(RUN_OBJ)
	@$(call MKDIR_P,$(BIN_DIR))
	$(CC) $(RUN_OBJ) $(LIBARM7VM) -o $@
	@echo Built $(ARM7_RUN)

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(BIOS_HEADER)
	@$(call MKDIR_P,$(BUILD_DIR))
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIOS_BIN):
	$(MAKE) -C ../../firmware/bios

buildbios:
	$(MAKE) -C ../../firmware/bios
	$(PYTHON) $(EMBED_BIOS) $(BIOS_BIN) $(BIOS_HEADER)

$(BIOS_HEADER): $(BIOS_BIN) $(EMBED_BIOS)
	$(PYTHON) $(EMBED_BIOS) $(BIOS_BIN) $(BIOS_HEADER)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@$(call MKDIR_P,$(BUILD_DIR))
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

test: $(ARM7_RUN)
	$(PYTHON) $(TEST_DIR)/run_tests.py "$(ARM7_RUN)"

install: $(ARM7_RUN)
	@$(call MKDIR_P,$(BINDIR))
	@$(call COPY_FILE,$(ARM7_RUN),$(BINDIR)/arm7-run$(EXEEXT))
	@echo Installed arm7-run to $(BINDIR)

clean:
	@$(call RM_RF,$(BUILD_DIR))
	@$(call RM_RF,$(BIN_DIR))
	@$(call RM_RF,$(TEST_DIR)/build)

rebuild: clean all
