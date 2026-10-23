# Inspire du modele officiel numworks/epsilon-sample-app-c
Q ?= @
CC = arm-none-eabi-gcc
NWLINK = npx --yes -- nwlink@latest
BUILD_DIR = output

src = $(addprefix src/,main.c)

CFLAGS = -std=c99 -Os -Wall -ffunction-sections -fdata-sections
CFLAGS += -fno-builtin -fno-tree-loop-distribute-patterns
CFLAGS += $(shell $(NWLINK) eadk-cflags)
LDFLAGS = -Wl,--relocatable -nostartfiles --specs=nosys.specs
LDFLAGS += $(shell $(NWLINK) eadk-ldflags 2>/dev/null)

.PHONY: build run clean
build: $(BUILD_DIR)/jeux.nwa

$(BUILD_DIR)/jeux.nwa: $(src) src/icon.png
	@mkdir -p $(BUILD_DIR)
	@echo "BUILD   $@"
	$(Q) $(NWLINK) nwa-from-source $(CFLAGS) $(src) -o $@ --icon src/icon.png 2>/dev/null || \
	  ( $(CC) $(CFLAGS) $(LDFLAGS) $(src) -o $(BUILD_DIR)/jeux.elf && \
	    $(NWLINK) elf-to-nwa $(BUILD_DIR)/jeux.elf $@ --icon src/icon.png )

run: build
	$(NWLINK) install-nwa $(BUILD_DIR)/jeux.nwa

clean:
	rm -rf $(BUILD_DIR)
