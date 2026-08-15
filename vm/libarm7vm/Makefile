# libarm7vm - ARM7 VM core bridge + public API
#
# Layer 3 migration strategy:
#   Keep compiling the proven legacy CPU/HW/VM implementation while adding
#   an opaque public host-side API owned by vm/libarm7vm itself.
#
# This establishes the seam that arm7-run, arm7-runx, and later arm7-node
# can migrate toward before the legacy implementation files are relocated.

PROJECT := libarm7vm

ROOT       := ../..
LEGACY_SRC := $(ROOT)/src
SRC_DIR    := src
INC_DIR    := include
TEST_DIR   := tests
BUILD_DIR  := build
LIB_DIR    := lib

ifeq ($(OS),Windows_NT)
	CC := gcc
	AR := ar
	EXEEXT := .exe

	PREFIX ?= $(USERPROFILE)/.local
	LIBDIR ?= $(PREFIX)/lib
	INCLUDEDIR ?= $(PREFIX)/include

	MKDIR_P = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
	RM_RF = if exist "$(subst /,\,$1)" rmdir /S /Q "$(subst /,\,$1)"
	COPY_FILE = copy /Y "$(subst /,\,$1)" "$(subst /,\,$2)" >NUL
else
	CC ?= cc
	AR ?= ar
	EXEEXT :=

	PREFIX ?= $(HOME)/.local
	LIBDIR ?= $(PREFIX)/lib
	INCLUDEDIR ?= $(PREFIX)/include

	MKDIR_P = mkdir -p "$1"
	RM_RF = rm -rf "$1"
	COPY_FILE = cp -f "$1" "$2"
endif

CPPFLAGS ?= -I$(INC_DIR) -I$(LEGACY_SRC)/include -I$(LEGACY_SRC)
CFLAGS ?= -std=c11 -Wall -Wextra -O2

API_SRC  := $(wildcard $(SRC_DIR)/*.c)
CPU_SRC  := $(wildcard $(LEGACY_SRC)/cpu/*.c)
HW_SRC   := $(wildcard $(LEGACY_SRC)/hw/*.c)
VM_SRC   := $(wildcard $(LEGACY_SRC)/vm/*.c)
UTIL_SRC := $(wildcard $(LEGACY_SRC)/util/*.c)

API_OBJ  := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/api/%.o,$(API_SRC))
CPU_OBJ  := $(patsubst $(LEGACY_SRC)/cpu/%.c,$(BUILD_DIR)/cpu/%.o,$(CPU_SRC))
HW_OBJ   := $(patsubst $(LEGACY_SRC)/hw/%.c,$(BUILD_DIR)/hw/%.o,$(HW_SRC))
VM_OBJ   := $(patsubst $(LEGACY_SRC)/vm/%.c,$(BUILD_DIR)/vm/%.o,$(VM_SRC))
UTIL_OBJ := $(patsubst $(LEGACY_SRC)/util/%.c,$(BUILD_DIR)/util/%.o,$(UTIL_SRC))

OBJECTS := $(API_OBJ) $(CPU_OBJ) $(HW_OBJ) $(VM_OBJ) $(UTIL_OBJ)
LIBARM7VM := $(LIB_DIR)/libarm7vm.a
API_TEST := $(BUILD_DIR)/test_api$(EXEEXT)

PUBLIC_HEADER := $(INC_DIR)/arm7vm/arm7vm.h

.PHONY: all test install clean rebuild info

all: $(LIBARM7VM)

info:
	@echo Project: $(PROJECT)
	@echo Public include root: $(INC_DIR)
	@echo Public API sources: $(words $(API_SRC))
	@echo Legacy source root: $(LEGACY_SRC)
	@echo CPU sources: $(words $(CPU_SRC))
	@echo HW sources: $(words $(HW_SRC))
	@echo VM sources: $(words $(VM_SRC))
	@echo UTIL sources: $(words $(UTIL_SRC))
	@echo Objects: $(words $(OBJECTS))

$(LIBARM7VM): $(OBJECTS)
	@$(call MKDIR_P,$(LIB_DIR))
	$(AR) rcs $@ $^
	@echo Built $(LIBARM7VM)

$(BUILD_DIR)/api/%.o: $(SRC_DIR)/%.c $(PUBLIC_HEADER)
	@$(call MKDIR_P,$(BUILD_DIR)/api)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/cpu/%.o: $(LEGACY_SRC)/cpu/%.c
	@$(call MKDIR_P,$(BUILD_DIR)/cpu)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hw/%.o: $(LEGACY_SRC)/hw/%.c
	@$(call MKDIR_P,$(BUILD_DIR)/hw)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vm/%.o: $(LEGACY_SRC)/vm/%.c
	@$(call MKDIR_P,$(BUILD_DIR)/vm)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/util/%.o: $(LEGACY_SRC)/util/%.c
	@$(call MKDIR_P,$(BUILD_DIR)/util)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(API_TEST): $(TEST_DIR)/test_api.c $(LIBARM7VM) $(PUBLIC_HEADER)
	@$(call MKDIR_P,$(BUILD_DIR))
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_DIR)/test_api.c $(LIBARM7VM) -o $@

test: $(API_TEST)
	"$(API_TEST)"

install: $(LIBARM7VM) $(PUBLIC_HEADER)
	@$(call MKDIR_P,$(LIBDIR))
	@$(call MKDIR_P,$(INCLUDEDIR)/arm7vm)
	@$(call COPY_FILE,$(LIBARM7VM),$(LIBDIR)/libarm7vm.a)
	@$(call COPY_FILE,$(PUBLIC_HEADER),$(INCLUDEDIR)/arm7vm/arm7vm.h)
	@echo Installed libarm7vm.a to $(LIBDIR)
	@echo Installed public headers to $(INCLUDEDIR)/arm7vm

clean:
	@$(call RM_RF,$(BUILD_DIR))
	@$(call RM_RF,$(LIB_DIR))

rebuild: clean all
