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

#Project name.
PROJECT_NAME ?= wifiswitch
#Unique configuration signature (4 bytes).
ESP_CONFIG_SIG ?= 0xCF60BEEF
#Build OS type only linux for now.
BUILD_OS ?= linux
# name for the target project
TARGET = wifiswitch
# Baud rate of the ESP8266 serial console.
BAUD_RATE = 230400

### Project directory configuration. ###
# Modules (subdirectories) of the project to include when compiling.
MODULES	= user user/fs user/net user/slighttp user/tools user/driver
MODULES	+= user/handlers/fs user/handlers/deny user/handlers/rest
MODULES += user/config
# Directory to use when creating the file system image.
FS_DIR := fs/
# Directory to copy log files of the ESP8266 serial output to.
LOG_DIR := logs
# Directory with custom build tools.
TOOLS_DIR := tools

### Set defines for the different build time debug configuration variables. ###
# Debug output on serial.
ifdef DEBUG
ESP_CFLAGS += -DDEBUG
endif
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

#Add project name
ESP_CFLAGS += -DPROJECT_NAME="$(PROJECT_NAME)"

#Configuration signature.
ESP_CFLAGS += -DESP_CONFIG_SIG=$(ESP_CONFIG_SIG)



