# Build time configuration for ESP8266 projects.
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

# Project name.
PROJECT_NAME = wifiswitch
# Version x.y.z.
VERSION ?= 0.2.0
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

### Project directory configuration. ###
# Modules (subdirectories) of the project to include when compiling.
MODULES	= user user/fs user/net user/slighttp user/tools user/driver
MODULES	+= user/handlers/fs user/handlers/deny user/handlers/rest
MODULES += user/config
# Libraries used in this project, mainly provided by the SDK
LIBS = c gcc hal pp phy net80211 lwip wpa main json
# Directory to use when creating the file system image.
FS_DIR := fs/
# Directory to copy log files of the ESP8266 serial output to.
LOG_DIR := logs
# Directory with custom build tools.
TOOLS_DIR := tools

### Set defines for the different build time debug configuration variables. ###
# Warnings on serial.
ifdef WARNINGS
	ESP_CFLAGS += -DWARNINGS
endif
# SDK debug output on serial.
ifdef SDK_DEBUG
	ESP_CFLAGS += -DSDK_DEBUG
endif
# Memory de-/allocation info on serial.
ifdef DEBUG_MEM
	ESP_CFLAGS += -DDEBUG_MEM
endif
# List memory allocations info on serial.
ifdef DEBUG_MEM_LIST
	ESP_CFLAGS += DEBUG_MEM_LIST
endif

# Configuration signature.
ESP_CFLAGS += -DESP_CONFIG_SIG=$(ESP_CONFIG_SIG)

ifdef DEBUG
	VERSION := $(VERSION)+debug
endif
#Version
ESP_CFLAGS += -DVERSION='"$(VERSION)"'

# Configuration signature.
ESP_CFLAGS += -DESP_CONFIG_SIG=$(ESP_CONFIG_SIG)

