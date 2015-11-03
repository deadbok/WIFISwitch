# Build time configuration for building ESP8266 projects linux.
# 
# Based on work by:
# - zarya
# - Jeroen Domburg (Sprite_tm)
# - Christian Klippel (mamalala)
# - Tommie Gannert (tommie)
#
# ChangeLog:
#
#  - 2015-10-07: Initial version.
#  - 2015-10-09: Added LOG_FILE.
#              : Added debug rule.

### Toolchain configuration. ###
# Base directory for the compiler.
XTENSA_TOOLS_ROOT ?= /home/oblivion/esp8266/esp-open-sdk/xtensa-lx106-elf/bin/
# Base directory of the ESP8266 SDK package, absolute.
SDK_BASE ?= /home/oblivion/esp8266/esp-open-sdk/sdk/
# Select which tools to use as compiler, librarian and linker
CC := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
AR := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
OBJCOPY := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objcopy
OBJDUMP := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objdump
READELF := $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-readelf

### Build configuration. ###
# Path for compiler to write object files.
BUILD_BASE = build
# Path to write the finished firmware build.
FW_BASE	= firmware
# Serial log file name.
LOG_FILE = debug-$(shell date +%Y-%m-%d-%H-%M-%S).log

### Version information from github ###
# Stolen from esp-link Makefile (https://github.com/jeelabs/esp-link/blob/master/Makefile)
GIT_DATE := $(shell date '+%F %T')
GIT_BRANCH := $(shell if git diff --quiet HEAD; then git describe --tags; \
              else git symbolic-ref --short HEAD; fi)
GIT_SHA := $(shell if git diff --quiet HEAD; then git rev-parse --short HEAD | cut -d"/" -f 3; \
           else echo "development"; fi)
GIT_VERSION ?= $(GIT_BRANCH) - $(GIT_DATE) - $(GIT_SHA)
# Tell compiler about it.
ESP_CFLAGS += -DGIT_VERSION='"$(GIT_VERSION)"'

### esptool.py ESP8266 flash tool configuration.
# Path.
ESPTOOL	:= python2 $(SDK_BASE)../esptool/esptool.py
# Serial port.
ESPPORT	:= /dev/ttyUSB0
# Programming baud rate.
ESPSPEED := 230400

#Set verbose flags on DEBUG.
ifdef DEBUG
 VFLAG = -v
endif

### DBFFS configuration. ###
FS_CREATE := ./$(TOOLS_DIR)/dbffs-image
DBFFS_CREATE := $(FS_CREATE) $(VFLAG)

### ESP8266 firmware binary configuration. ###
GEN_CONFIG := ./$(TOOLS_DIR)/gen_config $(VFLAG)

### Misc tools. ###
# Copy.
CP := cp $(VFLAG)
#Create directory.
MKDIR := mkdir $(VFLAG) -p
#Delete file.
RM := rm $(VFLAG) -f
#Echo.
ECHO := @echo
#Change directory.
CD := cd
#Get size of file.
FILESIZE := stat --printf="%s"
#Serial terminal cmd.
TERM_CMD = minicom -D $(ESPPORT) -o -b $(BAUD_RATE) -C ./debug.log
