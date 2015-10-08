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

#Include project configuration.
include mk/config.mk
#Include OS specific configuration.
include mk/$(BUILD_OS).mk

# Compile flags.
ifndef DEBUG
CFLAGS = -O2 -std=c99 -Wall -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -DDB_ESP8266 -D__ets__ -DICACHE_FLASH
else
CFLAGS = -g -std=c99 -Wall -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -DDB_ESP8266 -D__ets__ -DICACHE_FLASH
endif

# Linker flags used to generate the main object file
LDFLAGS	= -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

# Linker script used for the above linker step.
LD_SCRIPT	= eagle.app.v6.ld

# Various paths from the SDK used in this project
SDK_LIBDIR	= lib
SDK_LDDIR	= ld
SDK_INCDIR	= include include/json

# We create two different files for uploading into the flash
# These are the names and addresses of these.
FW_FILE_1_ADDR	= 0x00000
FW_FILE_2_ADDR	= 0x40000

####
#### no user configurable options below here
####
SRC_DIR		 := $(MODULES)
BUILD_DIR	 := $(addprefix $(BUILD_BASE)/,$(MODULES))

SDK_LIBDIR	 := $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR	 := $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))

SRC			 := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
OBJ			 := $(patsubst %.c,$(BUILD_BASE)/%.o,$(SRC))
LIBS		 := $(addprefix -l,$(LIBS))
APP_AR		 := $(addprefix $(BUILD_BASE)/,$(TARGET)_app.a)
TARGET_OUT	 := $(addprefix $(BUILD_BASE)/,$(TARGET).out)
FS_FILES	 := $(shell find $(FS_DIR) -type f -name '*')

LD_SCRIPT		:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT))

INCDIR			:= $(addprefix -I,$(SRC_DIR))
EXTRA_INCDIR	:= $(addprefix -I,$(EXTRA_INCDIR))
MODULE_INCDIR	:= $(addsuffix /include,$(INCDIR))

FW_FILE_1		:= $(addprefix $(FW_BASE)/,$(FW_FILE_1_ADDR).bin)
FW_FILE_2		:= $(addprefix $(FW_BASE)/,$(FW_FILE_2_ADDR).bin)

# Get size of the file system image
FS_SIZE = $(shell printf '%d\n' $$($(FILESIZE) $(FW_FILE_FS) ))

# File system image flash location, just after the code.
FS_START_OFFSET = $(shell printf '0x%X\n' $$(( ($$($(FILESIZE) $(FW_FILE_1)) + 0x4000) & (0xFFFFC000) )) )
# File system highest address.
FS_END = 0x2E000
# Maximum image size.
FS_MAX_SIZE = $(shell printf '%d\n' $$(($(FS_END) - $(FS_START_OFFSET) - 1)))
#File system image file name.
FW_FILE_FS = $(FW_BASE)/$(FS_START_OFFSET).fs

vpath %.c $(SRC_DIR)
vpath %.h $(SRC_DIR)

define compile-objects
$1/%.o: %.c
	$(CC) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS) -MD -Wa,-ahls=$(basename $$@).lst -c $$< -o $$@
endef

.PHONY: all checkdirs flash flashblank clean debug debugflash docs flashfs

all: checkdirs $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2) $(FW_FILE_FS)
	@./$(TOOLS_DIR)/mem_usage.sh $(TARGET_OUT) 81920
	$(ECHO) File system start offset: $(FS_START_OFFSET)
	$(ECHO) File system end offset: $(FS_END)
	$(ECHO) File system size: $(FS_SIZE)
	$(ECHO) File system maximum size: $(FS_MAX_SIZE)
	
$(FW_BASE)/%.bin: $(TARGET_OUT) | $(FW_BASE)
	$(ESPTOOL) elf2image -o $(FW_BASE)/ $(TARGET_OUT)

$(TARGET_OUT): $(APP_AR)
	$(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group,-Map,$(basename $@).map -o $@

$(APP_AR): $(OBJ)
	$(AR) cru $@ $^

checkdirs: $(BUILD_DIR) $(FW_BASE) $(LOG_DIR) $(TOOLS_DIR)

$(BUILD_DIR):
	$(MKDIR) $@

$(FW_BASE):
	$(MKDIR) $@

$(LOG_DIR):
	$(MKDIR) $@

$(TOOLS_DIR):
	$(MKDIR) $@
	
flash: all 
	./$(TOOLS_DIR)/testsize.sh $(FW_FILE_FS) $(FS_MAX_SIZE)
	$(ESPTOOL) --port $(ESPPORT) -b $(ESPSPEED) write_flash $(FW_FILE_1_ADDR) $(FW_FILE_1) $(FW_FILE_2_ADDR) $(FW_FILE_2) $(FS_START_OFFSET) $(FW_FILE_FS)
	
flashblank:
	$(ESPTOOL) --port $(ESPPORT) -b $(ESPSPEED) write_flash 0x7E000 bin/blank.bin

clean:
	$(RM) -R $(FW_BASE) $(BUILD_BASE)
	$(MAKE) -C tools/dbffs-tools clean

debug: $(LOG_DIR)
#Remove the old log
	> ./debug.log
	minicom -D $(ESPPORT) -o -b $(BAUD_RATE) -C ./debug.log
#Make a copy of the new log before with a saner name.
	cp ./debug.log $(LOG_DIR)/debug-$(shell date +%Y-%m-%d-%H-%M-%S).log

debugflash: flash debug

docs: doxygen

doxygen: .doxyfile
	doxygen .doxyfile
	$(MAKE) -C tools/dbffs-tools/

.PHONY: $(FS_CREATE)
$(FS_CREATE):
	$(MAKE) -C tools/dbffs-tools all install

$(FW_FILE_FS): $(FS_FILES) $(FS_CREATE)
	$(DBFFS_CREATE) $(FS_DIR) $(FW_FILE_FS)
	
flashfs: $(FW_FILE_FS)
	tools/testsize.sh $(FW_FILE_FS) $(FS_MAX_SIZE)
	$(ESPTOOL) --port $(ESPPORT) -b $(ESPSPEED) write_flash $(FS_START_OFFSET) $(FW_FILE_FS)

$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))

#Include .d files with targets and dependencies.
-include $(patsubst %.c,$(BUILD_BASE)/%.d,$(SRC))
