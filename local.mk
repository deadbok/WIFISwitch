include mk/linux.mk

#From: John Graham-Cumming
#	   http://blog.jgc.org/2011/07/gnu-make-recursive-wildcard-function.html
#Recursivly finds all files in a directory and all subdirectories
#rwildcard=$(shell echo "Colecting files $1; $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d)))
rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

# Project name.
PROJECT_NAME = wifiswitch
# Version x.y.z.
PROJECT_VERSION = 0.2.0
# Unique configuration signature (4 bytes).
ESP_CONFIG_SIG = 0xCF60BEEF
# Build OS type only linux for now.
BUILD_OS ?= linux
# Name of the project target.
TARGET = wifiswitch
# Baud rate of the ESP8266 serial console.
BAUD_RATE ?= 230400
# Flash size, 512 or 1024.
FLASH_SIZE ?= 512
# Network mode to start in.
# *  1 Client.
# *  2 AP.
NETWORK_MODE = 1
# Default hostname
HOSTNAME ?= wifiswitch

### Project directory configuration. ###
# Directory for generating the file system root. A Makefile is expected
# reside in this directory, that does whatever is needed, before creating
# the image.
FS_DIR = fs/
# Directory to use, as root, when creating the file system image.
FS_ROOT_DIR = fs/root_out/
# Directory to use as source, when generating file system root.
FS_SRC_DIR = fs/root_src/
# Directory to copy log files of the ESP8266 serial output to.
LOG_DIR = logs
# Directory with custom build tools.
TOOLS_DIR = tools
# Where to put the final firmware binaries.
FW_BASE = $(PROGRAM_DIR)firmware/

### Firmware image configuration. ###
ifeq ("$(FLASH_SIZE)","4")
	# Max size of any of the firmware files. 512Kb / 2 - 4KB at the start - 16Kb at the end.
	FW_FILE_MAX := 241664
else 
	$(error Flash size not supported.)
endif
FW_FILE_1_SIZE = $(call filesize,$(FW_FILE_1))

### File system variables. ###
# File system image is flashed 4KB aligned just after the resident code.
FW_FILE_1_SIZE = $(call filesize,$(FW_FILE_1))
# File system highest possible address (config data comes after).
FS_END = 0x3b000
# Maximum image size.
FS_MAX_SIZE = $(shell printf '%d\n' $$(($(FS_END) - $(FS_FILE_ADDR) - 1)))
# File system image file name and path.
FW_FILE_FS = $(FW_BASE)/www.dbffs
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
FW_FILE_CONFIG = $(addprefix $(FW_BASE)/,$(FW_FILE_CONFIG_ADDR).cfg)

### Exports for sub-make. ###
export DEBUG
export DEBUGFS
export PROJECT_NAME
export ESP_CONFIG_SIG
export BAUD_RATE

## Default to no Over The Air update.
OTA ?= 0

### Add project defines to CFLAGS
EXTRA_C_CXX_FLAGS += -DESP_CONFIG_SIG=$(ESP_CONFIG_SIG) -DDB_ESP8266 -DPROJECT_VERSION='"$(PROJECT_VERSION)"' -DPROJECT_NAME='"$(PROJECT_NAME)"' -DBAUD_RATE=$(BAUD_RATE) 
ifdef DEBUG
	EXTRA_C_CXX_FLAGS += -DDEBUG
endif

### Build the file system tools. ###
.PHONY: $(DBFFS_CREATE)
$(DBFFS_CREATE):
	$(Q) $(MAKE) -C tools/dbffs-tools all install
	
### ESP8266 firmware binary configuration. ###
GEN_CONFIG := ./$(TOOLS_DIR)/gen_config $(VFLAG)
# Build the configuration tools.
.PHONY: $(GEN_CONFIG)
$(GEN_CONFIG):
	$(Q) $(MAKE) -C tools/esp-config-tools all install

# Override esp-open-rtos all target.
all: $(PROGRAM_OUT) $(FW_FILE_1) $(FW_FILE_2) $(FW_FILE_CONFIG) $(FW_FILE_FS) $(FW_FILE)
	$(Q) ./$(TOOLS_DIR)/mem_usage.sh $(PROGRAM_OUT) 81920 $(FW_FILE_MAX)
	$(Q) $(ECHO) File system uses $(FS_SIZE) bytes of ROM, $$(( $(FS_MAX_SIZE) - $(FS_SIZE) )) bytes free of $(FS_MAX_SIZE).
	$(Q) $(ECHO) Resident firmware size $(FW_FILE_1_SIZE) bytes of ROM, $$(( $(FW_FILE_MAX) - $(FW_FILE_1_SIZE) )) bytes free of $(FW_FILE_MAX).
