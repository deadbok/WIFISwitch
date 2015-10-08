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
# Directory to use when creating the file system image.
FS_DIR := fs/
# Directory to copy log files of the ESP8266 serial output to.
LOG_DIR		 := logs
# Directory with custom build tools.
TOOLS_DIR	 := tools
