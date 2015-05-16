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

# Output directors to store intermediate compiled files
# relative to the project directory
BUILD_BASE	= build
FW_BASE		= firmware

# base directory for the compiler
XTENSA_TOOLS_ROOT ?= /home/oblivion/esp-open-sdk/xtensa-lx106-elf/bin/

# base directory of the ESP8266 SDK package, absolute
SDK_BASE	?= /home/oblivion/esp-open-sdk/sdk/

# esptool.py path and port
ESPTOOL		?= esptool.py
ESPPORT		?= /dev/ttyAMA0
ESPSPEED	?= 921600

# name for the target project
TARGET		= wifiswitch

# which modules (subdirectories) of the project to include in compiling
MODULES		= spiffs/src user
EXTRA_INCDIR    = include

# Directory to use when creating the file system image.
FS_DIR		?= fs

# libraries used in this project, mainly provided by the SDK
LIBS		= c gcc hal pp phy net80211 lwip wpa main

# compiler flags using during compilation of source files
CFLAGS		= -Os -g -O2 -std=c99 -Wall -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DICACHE_FLASH

# linker flags used to generate the main object file
LDFLAGS		= -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

# linker script used for the above linkier step
LD_SCRIPT	= eagle.app.v6.ld

# various paths from the SDK used in this project
SDK_LIBDIR	= lib
SDK_LDDIR	= ld
SDK_INCDIR	= include include/json

# we create two different files for uploading into the flash
# these are the names and options to generate them
FW_FILE_1_ADDR	= 0x00000
FW_FILE_2_ADDR	= 0x40000

# select which tools to use as compiler, librarian and linker
CC		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
AR		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc


####
#### no user configurable options below here
####
SRC_DIR		:= $(MODULES)
BUILD_DIR	:= $(addprefix $(BUILD_BASE)/,$(MODULES))
LOG_DIR		:= logs
TOOLS_DIR	:= tools

SDK_LIBDIR	:= $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR	:= $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))

SRC			:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
OBJ			:= $(patsubst %.c,$(BUILD_BASE)/%.o,$(SRC))
LIBS		:= $(addprefix -l,$(LIBS))
APP_AR		:= $(addprefix $(BUILD_BASE)/,$(TARGET)_app.a)
TARGET_OUT	:= $(addprefix $(BUILD_BASE)/,$(TARGET).out)

LD_SCRIPT		:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT))

INCDIR			:= $(addprefix -I,$(SRC_DIR))
EXTRA_INCDIR	:= $(addprefix -I,$(EXTRA_INCDIR))
MODULE_INCDIR	:= $(addsuffix /include,$(INCDIR))

FW_FILE_1		:= $(addprefix $(FW_BASE)/,$(FW_FILE_1_ADDR).bin)
FW_FILE_2		:= $(addprefix $(FW_BASE)/,$(FW_FILE_2_ADDR).bin)
FW_FS			:= $(FW_BASE)/spiffs.bin

vpath %.c $(SRC_DIR)
vpath %.h $(SRC_DIR)

define compile-objects
$1/%.o: %.c
	$(CC) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS) -MD -c $$< -o $$@
endef

.PHONY: all checkdirs flash flashblank clean debug debugflash docs spiffy/build/spiffy

all: checkdirs $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2) $(FW_FS)
	@./mem_usage.sh $(TARGET_OUT) 81920

$(FW_BASE)/%.bin: $(TARGET_OUT) | $(FW_BASE)
	$(ESPTOOL) elf2image -o $(FW_BASE)/ $(TARGET_OUT)

$(TARGET_OUT): $(APP_AR)
	$(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@

$(APP_AR): $(OBJ)
	$(AR) cru $@ $^

checkdirs: $(BUILD_DIR) $(FW_BASE)

$(BUILD_DIR):
	mkdir -p $@

$(FW_BASE):
	mkdir -p $@

$(LOG_DIR):
	mkdir -p $@

$(TOOLS_DIR):
	mkdir -p $@
	
flash: all 
	$(ESPTOOL) --port $(ESPPORT) -b $(ESPSPEED) write_flash $(FW_FILE_1_ADDR) $(FW_FILE_1) $(FW_FILE_2_ADDR) $(FW_FILE_2)
	
flashblank:
	$(ESPTOOL) --port $(ESPPORT) -b $(ESPSPEED) write_flash 0x7E000 bin/blank.bin

clean:
	rm -rf $(FW_BASE) $(BUILD_BASE)
	$(MAKE) -C spiffy clean 

debug: $(LOG_DIR) all
#Remove the old log
	> ./debug.log
	minicom -D $(ESPPORT) -o -b 115200 -C ./debug.log
#Make a copy of the new log before with a saner name.
	cp ./debug.log $(LOG_DIR)/debug-$(shell date +%Y-%m-%d-%H-%M-%S).log

debugflash: flash debug

docs: doxygen

doxygen: .doxyfile
	doxygen .doxyfile
	
spiffy/spiffy:
	$(MAKE) -C spiffy
	
$(TOOLS_DIR)/spiffy: $(TOOLS_DIR) spiffy/spiffy
	cp spiffy/spiffy $@
	
$(FW_FS): $(TOOLS_DIR)/spiffy
	tools/spiffy $(FS_DIR) $@ 16384
	
$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))

#Include .d files with targets and dependencies.
-include $(patsubst %.c,$(BUILD_BASE)/%.d,$(SRC))
