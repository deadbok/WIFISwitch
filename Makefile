PROGRAM = wifiswitch
PROGRAM_DIR := ./
PROGRAM_SRC_DIR = src src/driver src/config src/fs src/tools 
PROGRAM_EXTRA_SRC_FILES = src/net/wifi.c src/net/dhcpserver.c

FLASH_SIZE = 4

ESPBAUD = 230400

ifdef DEBUG
	FLAVOR := debug
endif

# C_CXX_FLAGS = -Wall -Wl,-EL -nostdlib $(EXTRA_C_CXX_FLAGS)

ESP_OPEN_RTOS_PATH = /home/oblivion/esp8266/esp-open-rtos
include $(ESP_OPEN_RTOS_PATH)/common.mk

############# Project specific make rules and overrides. ###############

INC_DIRS := $(PROGRAM_DIR)src/include $(INC_DIRS)

#### Change firmware loc to 0x40000 to make room for fs and config. ####
# Firmware address in flash.
FW_ADDR_2 = 0x40000
FW_FILE_2 = $(addprefix $(FW_BASE),$(FW_ADDR_2).bin)

#Override
ifeq ($(OTA),0)
LINKER_SCRIPTS  = $(PROGRAM_DIR)ld/nonota.ld
else
LINKER_SCRIPTS = $(PROGRAM_DIR)ld/ota.ld
endif
LINKER_SCRIPTS += $(ROOT)ld/common.ld $(ROOT)ld/rom.ld

#### Create directories. ####
$(FS_ROOT_DIR) $(LOG_DIR) $(TOOLS_DIR):
	$(Q) $(MKDIR) $@

#### Flash file system stuff. ####
# Generate file system. (pathsubst is to force stuff to happen, when there are no files in the root yet.)
$(FS_FILES): $(FS_SRC_FILES)
	$(Q) $(MAKE) -C $(FS_DIR)

# Build the file system image.
$(FW_FILE_FS): $(FS_FILES) $(DBFFS_CREATE) $(FW_FILE_1)
	$(Q) $(DBFFS_CREATE) $(FS_ROOT_DIR) $(FW_FILE_FS)
	$(Q) $(ECHO) File system start offset: $(FS_FILE_ADDR), end offset: $(FS_END), maximum size $(FS_MAX_SIZE) bytes.

# Flash file system.	
flashfs: $(FW_FILE_FS)
	$(Q) tools/testsize.sh $(FW_FILE_FS) $(FS_MAX_SIZE)
	$(Q) $(ECHO) File system uses $(FS_SIZE) bytes of ROM, $$(( $(FS_MAX_SIZE) - $(FS_SIZE) )) bytes free of $(FS_MAX_SIZE).
	$(Q) $(ESPTOOL) --port $(ESPPORT) -b $(ESPSPEED) write_flash $(FS_FILE_ADDR) $(FW_FILE_FS)

#### ESP firmware binary configuration stuff. ####
### Generate configuration data. ###
$(FW_FILE_CONFIG): $(GEN_CONFIG) $(FW_FILE_1) $(FW_BASE)
	$(Q) $(GEN_CONFIG) $(FW_FILE_CONFIG) $(FS_FILE_ADDR) $(NETWORK_MODE) $(HOSTNAME)

# Flash configuration data.
flashconfig: $(FW_FILE_CONFIG)
	$(Q) tools/testsize.sh $(FW_FILE_CONFIG) 12288
	$(Q) $(ESPTOOL) --port $(ESPPORT) -b $(ESPBAUD) write_flash $(FW_FILE_CONFIG_ADDR) $(FW_FILE_CONFIG)

$(FW_FILE_1) $(FW_FILE_2): $(PROGRAM_OUT) $(FW_BASE)
	$(vecho) "FW $@"
	$(Q) $(ESPTOOL) elf2image $(ESPTOOL_ARGS) $< -o $(FW_BASE)
	
#Override general flash rule.
flashall: all
	$(Q) $(ESPTOOL) -p $(ESPPORT) --baud $(ESPBAUD) write_flash $(ESPTOOL_ARGS) $(FW_ADDR_2) $(FW_FILE_2) $(FW_ADDR_1) $(FW_FILE_1) $(FW_FILE_CONFIG_ADDR) $(FW_FILE_CONFIG) $(FS_FILE_ADDR) $(FW_FILE_FS)

#### Debugging. ####
# Run minicom and save serial output in LOG_DIR.
debug: $(LOG_DIR) all
# Remove the old log
	> ./debug.log
	$(Q) $(TERM_CMD)
# Make a copy of the new log before with a saner name.
	$(Q) $(CP) ./debug.log $(LOG_DIR)/$(LOG_FILE)
	
# Clean rules.
.PHONY: distclean clean

clean:
	$(Q) $(RM) -r $(BUILD_DIR)
	$(Q) $(RM) -r $(FW_BASE)
	$(Q) $(RM) -R $(FW_BASE) $(BUILD_DIR)
	$(Q) $(RM) $(DBFFS_CREATE)
	$(Q) $(RM) $(GEN_CONFIG)
	$(Q) $(MAKE) -C fs clean

# Clean everything
distclean: clean
	$(Q) $(MAKE) -C tools/dbffs-tools clean
	$(Q) $(MAKE) -C tools/esp-config-tools clean
	$(Q) $(RM) $(DBFFS_CREATE)
	$(Q) $(RM) $(GEN_CONFIG)
	$(Q) $(MAKE) -C fs distclean
	$(Q) $(RM) -r $(BUILD_DIR)
	$(Q) $(RM) -r $(FW_BASE)
	
#### Documentation stuff. ####
# Generate docs.
docs: doxygen

# Generate doxygen API docs.
.PHONY: doxygen
doxygen: .doxyfile
	$(Q) doxygen .doxyfile
	$(Q) $(MAKE) -C tools/dbffs-tools/ docs
	$(Q) $(MAKE) -C tools/esp-config-tools docs
