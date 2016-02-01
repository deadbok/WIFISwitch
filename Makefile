# Makefile for ESP8266 projects
#
# Thanks to:
# - zarya
# - Jeroen Domburg (Sprite_tm)
# - Christian Klippel (mamalala)
# - Tommie Gannert (tommie)
# - Martin Bo Kristensen Grønholdt (deadbok)
#
# Changelog:
# - 2014-10-06: Changed the variables to include the header file directory
# - 2014-10-06: Added global var for the Xtensa tool root
# - 2014-11-23: Updated for SDK 0.9.3
# - 2014-12-25: Replaced esptool by esptool.py
# - 2015-04-21: Maxed out verbosity for religious reasons.
# - 2015-05-03: Auto dependency generation.
# - 2015-05-03: debugflash target, that flashes firmware and immediately 
#				starts minicom.
# - 2015-05-07: docs target, to create HTML documentation with Doxygen.
# - 2015-05-09: debug target to just run minicom.
# - 2015-05-09: Saving of debug logs in separate files under LOG_DIR.
# - 2015-10-07: Cleanup and split into multiply files.
# - 2015-10-09: Add the -L switch to SDK_LIBDIR when generated.
# - 2015-10-09: Removed archive generation step.
# - 2015-10-09: Lots of comments.
# - 2015-11-03: Added distclean rule.
# - 2015-12-09: Complete rewrite, since the "recursive magic" faild.

#From: John Graham-Cumming
#	   http://blog.jgc.org/2011/07/gnu-make-recursive-wildcard-function.html
#Recursivly finds all files in a directory and all subdirectories
#rwildcard=$(shell echo "Colecting files $1; $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d)))
rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

# Initial Compiler flags.
ESP_CFLAGS =

# Include project configuration.
include mk/config.mk
# Include OS specific configuration.
include mk/$(BUILD_OS).mk

### Source files.
#Start with no source files.
C_SRC =
AS_SRC	= 
#Include per directory Make files, that will add source files to the
#above variables.
ESP_MAKEFILES = $(call rwildcard,$(SRC_DIR),*/esp.mk)
include $(ESP_MAKEFILES)

### Compiler and linker configuration. ###
# Compile flags.
ESP_CFLAGS += -Os -std=gnu99 -Wall -Wl,-EL -ffunction-sections -fdata-sections -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -DDB_ESP8266 -D__ets__ -DICACHE_FLASH -DPROJECT_NAME='"$(PROJECT_NAME)"' -DBAUD_RATE=$(BAUD_RATE)
# Linker flags used to generate the main object file
ESP_LDFLAGS = -nostdlib -Wl,--gc-sections -Wl,--no-check-sections -u call_user_start -Wl,-static
# Linker script used for the above linker step.
ESP_LD_SCRIPT	= eagle.app.v6.ld
# Possibly enable debug flag.
ifdef DEBUG
ESP_CFLAGS += -DDEBUG -ggdb -Og
endif

### SDK directories used to build the firmware. ###
# Library directory.
SDK_LIBDIR	= lib
# Linker scripts directory.
SDK_LDDIR	= ld
# Include file directories.
SDK_INCDIR = include include/json

### Firmware image configuration. ###
ifeq ("$(FLASH_SIZE)","512")
	# These two files contain the actual program code.
	# Flash address and file name of firmware resident code.
	FW_FILE_1_ADDR := 0x00000
	FW_FILE_1 = $(addprefix $(FW_BASE)/,$(FW_FILE_1_ADDR).bin)
	# Flash address and file name of firmware ROM code.
	FW_FILE_2_ADDR := 0x40000
	FW_FILE_2 = $(addprefix $(FW_BASE)/,$(FW_FILE_2_ADDR).bin)
	# Max size of any of the firmware files. 512Kb / 2 - 4KB at the start - 16Kb at the end.
	FW_FILE_MAX := 241664
else 
	$(error Flash size not supported.)
endif
FW_FILE_1_SIZE = $(call filesize,$(FW_FILE_1))

### Internal paths. Should not need modification. ### 
# Add the SDK base path to generate include and lib flags.
SDK_LIBDIR := $(addprefix -L$(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR := $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))
# Create include dir flag
INCDIR = $(addprefix -I,$(INCLUDE_DIR))
#List of directories under build/
BUILD_DIRS := $(addprefix $(BUILD_DIR)/,$(dir $(ESP_MAKEFILES)))
# Generate object file names from source file names.
OBJ	:= $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRC))
OBJ	+= $(patsubst %.S,$(BUILD_DIR)/%.o,$(AS_SRC))
# Add -l prefix to generate link flags.
LIBS := $(addprefix -l,$(LIBS))
# Add path and flag to generate linker flags.
ESP_LD_SCRIPT := $(addprefix -Tld/,$(ESP_LD_SCRIPT))
# Generate firmware file name.
TARGET := $(addprefix $(BUILD_DIR)/,$(TARGET))

### File system variables. ###
# File system image is flashed 4KB aligned just after the resident code.
# File system highest possible address.
FS_END = 0x3b000
# Maximum image size.
FS_MAX_SIZE = $(shell printf '%d\n' $$(($(FS_END) - $(FS_FILE_ADDR) - 1)))
# File system image file name and path.
FW_FILE_FS = $(FW_BASE)/www.fs
# Generate a list of source files for flash file system root.
FS_SRC_FILES = $(call rwildcard,$(FS_SRC_DIR),*)
# Generate a list of all files to be included in the flash file system.
FS_FILES = $(patsubst $(FS_SRC_DIR)%,$(FS_ROOT_DIR)%,$(FS_SRC_FILES))
# Get size of file system image.
FS_SIZE = $(call filesize,$(FW_FILE_FS))
# Calculate the 4Kb aligned address of the file system in flash.
FS_FILE_ADDR = $(shell printf '0x%X\n' $$(( ($(call filesize,$(FW_FILE_1)) + 0x4000) & (0xFFFFC000) )) ) 

### Config flash variables. ###
# Address to flash configuration data.
FW_FILE_CONFIG_ADDR := 0x3C000
# File name of the configuration data.
FW_FILE_CONFIG := $(addprefix $(FW_BASE)/,$(FW_FILE_CONFIG_ADDR).cfg)

### Exports for sub-make. ###
export DEBUG
export DEBUGFS
export PROJECT_NAME
export ESP_CONFIG_SIG
export BAUD_RATE

### Targets. ###
# Build c source files.
$(BUILD_DIR)/%.o: %.c
	$(CC) $(INCDIR) $(SDK_INCDIR) $(ESP_CFLAGS) -MD -Wa,-ahls=$(basename $@).lst -c $< -o $@
# Build assembler source files.
$(BUILD_DIR)/%.o: %.S
	$(CC) $(INCDIR) $(SDK_INCDIR) $(ESP_CFLAGS) -c $< -o $@

# Run these without checking if they're up to date.
.PHONY: all flash flashblank clean distclean debug debugflash docs flashfs flashconfig info

# Main build rule.
all: info $(BUILD_DIRS) $(OBJ) $(FW_FILE_1) $(FW_FILE_2) $(FW_FILE_FS) $(FW_FILE_CONFIG)
# TODO: Print only project entries.
	$(ECHO) "$(N_LIST_OBJECTS) largest functions:"
	$(ECHO) "   Num:    Value  Size Type    Bind   Vis      Ndx Name"
	@$(READELF) -s $(TARGET) | awk '$$4 == "FUNC" { print }' | sort -r -n -k 3 | head -n $(N_LIST_OBJECTS)
	$(ECHO) "$(N_LIST_OBJECTS) largest objects:"
	$(ECHO) "   Num:    Value  Size Type    Bind   Vis      Ndx Name"
	@$(READELF) -s $(TARGET) | awk '$$4 == "OBJECT" { print }' | sort -r -n -k 3 | head -n $(N_LIST_OBJECTS)
	./$(TOOLS_DIR)/mem_usage.sh $(TARGET) 81920 $(FW_FILE_MAX)
	$(ECHO) File system uses $(FS_SIZE) bytes of ROM, $$(( $(FS_MAX_SIZE) - $(FS_SIZE) )) bytes free of $(FS_MAX_SIZE).
	$(ECHO) Resident firmware size $(FW_FILE_1_SIZE) bytes of ROM, $$(( $(FW_FILE_MAX) - $(FW_FILE_1_SIZE) )) bytes free of $(FW_FILE_MAX).

# Generate the firmware image from the final ELF file.
$(FW_BASE)/%.bin: $(TARGET) | $(FW_BASE)
	$(ECHO)
	$(ECHO) --- CREATING FIRMWARE BINARY --- 
	$(ECHO)
	$(ESPTOOL) elf2image -o $(FW_BASE)/ $(TARGET)

# Link the target ELF file.
$(TARGET): $(OBJ)
	$(ECHO)
	$(ECHO) --- LINKING --- 
	$(ECHO)
	$(LD) $(SDK_LIBDIR) $(ESP_LD_SCRIPT) $(ESP_LDFLAGS) -Wl,--start-group $(LIBS) $(OBJ) -Wl,--end-group,-Map,$(basename $@).map -o $@
# This is for future OTA support.
#	$(OBJCOPY) --only-section .text -O binary $(TARGET) $(BUILD_BASE)/eagle.app.v6.text.bin
#	$(OBJCOPY) --only-section .data -O binary $(TARGET) $(BUILD_BASE)/eagle.app.v6.data.bin
#	$(OBJCOPY) --only-section .rodata -O binary $(TARGET) $(BUILD_BASE)/eagle.app.v6.rodata.bin
#	$(OBJCOPY) --only-section .irom0.text -O binary $(TARGET) $(BUILD_BASE)/eagle.app.v6.irom0text.bin

#### Flash file system stuff. ####
# Build the file system tools.
.PHONY: $(FS_CREATE)
$(FS_CREATE):
	$(MAKE) -C tools/dbffs-tools all install

# Generate file system. (pathsubst is to force stuff to happen, when there are no files in the root yet.)
$(FS_FILES): $(FS_SRC_FILES)
	$(ECHO)
	$(ECHO) --- CREATING FIRMWARE FILE SYSTEM ROOT --- 
	$(ECHO)
	$(MAKE) -C $(FS_DIR)

# Build the file system image.
$(FW_FILE_FS): $(FS_FILES) $(FS_CREATE) $(FW_FILE_1)
	$(ECHO)
	$(ECHO) --- CREATING FIRMWARE FILE SYSTEM IMAGE --- 
	$(ECHO) Files: $(FS_FILES)
	$(ECHO)
	$(DBFFS_CREATE) $(FS_ROOT_DIR) $(FW_FILE_FS)
	$(ECHO) File system start offset: $(FS_FILE_ADDR), end offset: $(FS_END), maximum size $(FS_MAX_SIZE) bytes.

# Flash file system.	
flashfs: $(FW_FILE_FS)
	tools/testsize.sh $(FW_FILE_FS) $(FS_MAX_SIZE)
	$(ECHO) File system uses $(FS_SIZE) bytes of ROM, $$(( $(FS_MAX_SIZE) - $(FS_SIZE) )) bytes free of $(FS_MAX_SIZE).
	$(ESPTOOL) --port $(ESPPORT) -b $(ESPSPEED) write_flash $(FS_FILE_ADDR) $(FW_FILE_FS)

#### ESP firmware binary configuration stuff. ####
# Build the configuration tools.
.PHONY: $(GEN_CONFIG)
$(GEN_CONFIG):
	$(MAKE) -C tools/esp-config-tools all install

# Generate configuration data.
$(FW_FILE_CONFIG): $(GEN_CONFIG) $(FW_FILE_FS)
	$(ECHO)
	$(ECHO) --- CREATING FIRMWARE CONFIG --- 
	$(ECHO)
	$(GEN_CONFIG) $(FW_FILE_CONFIG) $(FS_FILE_ADDR) $(NETWORK_MODE) $(HOSTNAME)

# Flash configuration data.
flashconfig: $(FW_FILE_CONFIG)
	tools/testsize.sh $(FW_FILE_CONFIG) 4096
	$(ESPTOOL) --port $(ESPPORT) -b $(ESPSPEED) write_flash $(FW_FILE_CONFIG_ADDR) $(FW_FILE_CONFIG)

#### Create directories. ####
$(BUILD_DIR) $(BUILD_DIRS) $(FW_BASE) $(FS_ROOT_DIR) $(LOG_DIR) $(TOOLS_DIR) $(SRC_DIRS):
	$(MKDIR) $@
	
#### Generic flashing. ####
#Flash everything.
flash: all
	$(ECHO)
	$(ECHO) --- FLASHING FIRMWARE --- 
	$(ECHO)
	./$(TOOLS_DIR)/testsize.sh $(FW_FILE_FS) $(FS_MAX_SIZE)
	$(ESPTOOL) --port $(ESPPORT) -b $(ESPSPEED) write_flash $(FW_FILE_1_ADDR) $(FW_FILE_1) $(FW_FILE_CONFIG_ADDR) $(FW_FILE_CONFIG) $(FW_FILE_2_ADDR) $(FW_FILE_2) $(FS_FILE_ADDR) $(FW_FILE_FS)

# Flash empty configuration values for the SDK.
flashblank:
	$(ECHO)
	$(ECHO) --- FLASHING BLANK SDK CONFIGURATION --- 
	$(ECHO)
	$(ESPTOOL) --port $(ESPPORT) -b $(ESPSPEED) write_flash 0x7E000 bin/blank.bin

# Flash preset configuration values for the SDK.
flashsdkconfig: bin/config.bin
	$(ECHO)
	$(ECHO) --- FLASHING PRESET SDK CONFIGURATION --- 
	$(ECHO)
	$(ESPTOOL) --port $(ESPPORT) -b $(ESPSPEED) write_flash 0x7D000 bin/config.bin
	
#### Do not keep it greasy. ####
# An end to the tears, and the in between years, and the troubles I've seen. 
clean:
	$(RM) -R $(FW_BASE) $(BUILD_DIR)
	$(RM) $(DBFFS_CREATE)
	$(RM) $(GEN_CONFIG)
	$(MAKE) -C fs clean
	
# Clean everything
distclean:
	$(RM) -R $(FW_BASE) $(BUILD_DIR)
	$(MAKE) -C tools/dbffs-tools clean
	$(MAKE) -C tools/esp-config-tools clean
	$(RM) $(DBFFS_CREATE)
	$(RM) $(GEN_CONFIG)
	$(MAKE) -C fs distclean

#### Debugging. ####
# Run minicom and save serial output in LOG_DIR.
debug: $(LOG_DIR)
# Remove the old log
	> ./debug.log
	$(TERM_CMD)
# Make a copy of the new log before with a saner name.
	$(CP) ./debug.log $(LOG_DIR)/$(LOG_FILE)

# Flash and start debugging.
debugflash: flash debug

#### Documentation stuff. ####
# Generate docs.
docs: doxygen

# Generate doxygen API docs.
doxygen: .doxyfile
	doxygen .doxyfile
	$(MAKE) -C tools/dbffs-tools/

# Print some initial project info.	
info:
	$(ECHO) Project: $(PROJECT_NAME) version $(VERSION)
	$(ECHO) Source dir: $(SRC_DIR)
	$(ECHO) Include dir: $(INCLUDE_DIR)
	$(ECHO) Build dir: $(BUILD_DIR)
	$(ECHO) Makefiles: $(ESP_MAKEFILES)
	$(ECHO)
	$(ECHO) --- BUILDING --- 
	$(ECHO)

# Include gcc generated .d files with targets and dependencies.
-include $(patsubst %.c,$(BUILD_BASE)/%.d,$(C_SRC))
