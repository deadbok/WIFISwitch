include mk/linux.mk

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
FS_DIR := fs/
# Directory to use, as root, when creating the file system image.
FS_ROOT_DIR := fs/root_out/
# Directory to use as source, when generating file system root.
FS_SRC_DIR := fs/root_src/
# Directory to copy log files of the ESP8266 serial output to.
LOG_DIR := logs
# Directory with custom build tools.
TOOLS_DIR := tools
# Where to put the final firmware binaries.
FW_BASE = $(PROGRAM_DIR)firmware/

### File system variables. ###
# File system image is flashed 4KB aligned just after the resident code.
# File system highest possible address (config data comes after).
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

### ESP8266 firmware binary configuration. ###
GEN_CONFIG := ./$(TOOLS_DIR)/gen_config $(VFLAG)
# Build the configuration tools.
.PHONY: $(GEN_CONFIG)
$(GEN_CONFIG):
	$(MAKE) -C tools/esp-config-tools all install

# Override esp-open-rtos all target.
all: $(PROGRAM_OUT) $(FW_FILE_1) $(FW_FILE_2) $(FW_FILE_CONFIG) $(FW_FILE)
