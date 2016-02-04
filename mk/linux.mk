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

### Build configuration. ###
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
EXTRA_C_CXX_FLAGS += -DGIT_VERSION='"$(GIT_VERSION)"'

#Set verbose flags on DEBUG.
ifdef DEBUG
 V = 1
 VFLAG = -v
else
 Q := @
endif

### DBFFS configuration. ###
FS_CREATE := ./$(TOOLS_DIR)/dbffs-image
DBFFS_CREATE := $(FS_CREATE) $(VFLAG)

### ESP8266 firmware binary configuration. ###
GEN_CONFIG := ./$(TOOLS_DIR)/gen_config $(VFLAG)

### Misc tools. ###
# Copy.
CP = $(Q) cp $(VFLAG)
#Create directory.
MKDIR = $(Q) mkdir $(VFLAG) -p
#Delete file.
RM = $(Q) rm $(VFLAG) -f
#Echo.
ECHO := @echo
#Change directory.
CD = $(Q) cd
#Concantate
CAT = $(Q) cat
#Get size of file.
filesize = $(shell stat -L --printf="%s" $(1))
#Serial terminal cmd.
TERM_CMD = minicom -D $(ESPPORT) -o -b $(BAUD_RATE) -C ./debug.log
