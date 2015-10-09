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
#  - 2015-10-07 Martin Grønholdt: Initial version.

### Toolchain configuration. ###
# Base directory for the compiler.
XTENSA_TOOLS_ROOT ?= /home/oblivion/esp8266/esp-open-sdk/xtensa-lx106-elf/bin/
# Base directory of the ESP8266 SDK package, absolute.
SDK_BASE ?= /home/oblivion/esp8266/esp-open-sdk/sdk/
# Libraries used in this project, mainly provided by the SDK
LIBS = c gcc hal pp phy net80211 lwip wpa main json
# Select which tools to use as compiler, librarian and linker
CC	:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
AR	:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD	:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc

### Build configuration. ###
# Path for compiler to write object files.
BUILD_BASE = build
# Path to write the finished firmware build.
FW_BASE	= firmware

### esptool.py ESP8266 flash tool configuration.
# Path.
ESPTOOL	:= python2 $(XTENSA_TOOLS_ROOT)/esptool.py
# Serial port.
ESPPORT	:= /dev/ttyUSB0
# Programming baud rate.
ESPSPEED := 230400

#Set verbose flags on DEBUG.
ifdef DEBUG
 VFLAG = -v
endif

### DBFFS configuration. ###
DBFFS_CREATE := ./$(TOOLS_DIR)/dbffs-image $(VFLAG)

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
