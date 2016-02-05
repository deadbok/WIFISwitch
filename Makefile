PROGRAM = wifiswitch
PROGRAM_DIR := ./
PROGRAM_SRC_DIR = user user/driver user/config

FLASH_SIZE = 4

ESPBAUD = 230400

FLAVOR = debug

# C_CXX_FLAGS = -Wall -Wl,-EL -nostdlib $(EXTRA_C_CXX_FLAGS)

ESP_OPEN_RTOS_PATH = /home/oblivion/esp8266/esp-open-rtos
include $(ESP_OPEN_RTOS_PATH)/common.mk

############# Project specific make rules and overrides. ###############

INC_DIRS := $(PROGRAM_DIR)user/include $(INC_DIRS)

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
	$(MKDIR) $@

### Generate configuration data. ###
$(FW_FILE_CONFIG): $(GEN_CONFIG) $(FW_BASE)
	$(GEN_CONFIG) $(FW_FILE_CONFIG) $(FS_FILE_ADDR) $(NETWORK_MODE) $(HOSTNAME)

# Flash configuration data.
flashconfig: $(FW_FILE_CONFIG)
	tools/testsize.sh $(FW_FILE_CONFIG) 12288
	$(ESPTOOL) --port $(ESPPORT) -b $(ESPBAUD) write_flash $(FW_FILE_CONFIG_ADDR) $(FW_FILE_CONFIG)
	
#Override general flash rule.
flash: $(FW_FILE_1) $(FW_FILE_2) $(FW_FILE_CONFIG)
	$(ESPTOOL) -p $(ESPPORT) --baud $(ESPBAUD) write_flash $(ESPTOOL_ARGS) $(FW_ADDR_2) $(FW_FILE_2) $(FW_ADDR_1) $(FW_FILE_1) $(FW_FILE_CONFIG_ADDR) $(FW_FILE_CONFIG)

#### Debugging. ####
# Run minicom and save serial output in LOG_DIR.
debug: $(LOG_DIR) all
# Remove the old log
	> ./debug.log
	$(TERM_CMD)
# Make a copy of the new log before with a saner name.
	$(CP) ./debug.log $(LOG_DIR)/$(LOG_FILE)
	
# Clean rules.
.PHONY: distclean clean

# Clean everything
distclean: clean
	$(MAKE) -C tools/dbffs-tools clean
	$(MAKE) -C tools/esp-config-tools clean
	$(RM) $(DBFFS_CREATE)
	$(RM) $(GEN_CONFIG)
	$(MAKE) -C fs distclean
	$(RM) -r $(BUILD_DIR)
	$(RM) -r $(FW_BASE)
