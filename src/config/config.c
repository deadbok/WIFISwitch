/** 
 * @file config.c
 *
 * @brief Routines for saving firmware configuration to the flash.
 * 
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@@ace2>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "main.h"
#include "debug.h"
#include "config/config.h"
#include "fs/int_flash.h"

/**
 * @brief Flash read wrapper for debug and error handling.
 * 
 * @param foff Address in flash to read from.
 * @param data Pointer to memory for the data.
 * @param len Number of bytes to read.
 */
static bool read_flash(uint32_t foff, void *data, uint16_t len)
{
	uint8_t ret;
	
	debug("Reading from flash at: 0x%x (%d byte(s)).\n", foff, len);
	 
	ret = sdk_spi_flash_read(foff, data, len);
	if (ret != SPI_FLASH_RESULT_OK)
	{
		error("Error reading flash: %d.\n", ret);
		return(false);
	}
	return(true);
}

/**
 * @brief Flash write wrapper for debug and error handling.
 * 
 * **This erases all of the 4Kb sector that data is written to.**
 */
static bool write_flash(uint32_t foff, void *data, uint16_t len)
{
	uint32_t sec_off;
	uint8_t ret;
	
	debug("Write to flash at: 0x%x (%d byte(s)).\n", foff, len);
	
	sec_off = foff >> 12;
	debug(" Sector address: 0x%x.\n", sec_off);
	vPortEnterCritical();
	ret = sdk_spi_flash_erase_sector(sec_off);
	vPortExitCritical();
	if (ret != SPI_FLASH_RESULT_OK)
	{
		error("Error erasing flash sector: %d.\n", ret);
		return(false);
	}

	taskYIELD();

	vPortEnterCritical(); 	
	ret = sdk_spi_flash_write(sec_off << 12, data, 4096);
	vPortExitCritical();
	if (ret != SPI_FLASH_RESULT_OK)
	{
		error("Error writing flash: %d.\n", ret);
		return(false);
	}
	return(true);
}

/**
 * @brief Load configuration data from flash.
 * 
 * Reimplementation of a function available in a later SDK.
 * 
 * @param fsec Sector offset in flash.
 * @param foff Offset in sector.
 * @param param Pointer to data buffer.
 * @param len Length of data to read.
 */
static bool sdk_system_param_load(uint16_t fsec, uint16_t foff, void *param, uint16_t len)
{
	uint32_t config_offset = 0;
	uint32_t config_sum = 0;
	uint32_t sum = 0;
	
	debug("Reading configuration from flash.\n");
	if (foff & 0x03)
	{
		error("Unaligned address.\n");
		return(false);
	}
	if (foff > SPI_FLASH_SEC_SIZE)
	{
		error("Address is beyond end of the sector.\n");
		return(false);
	}
	if (len & 0x03)
	{
		error("Unaligned length.\n");
		return(false);
	}	

	//Read copy offset and sum from the third sector.
	if (!read_flash((fsec + 2) * FLASH_SECTOR_SIZE, &config_offset,
				   sizeof(uint32_t)))
	{
		return(false);
	}
	debug(" Configuration offset: %d.\n", config_offset);

	if (!read_flash((fsec + 2) * FLASH_SECTOR_SIZE + sizeof(uint32_t),
					&config_sum, sizeof(uint32_t)))
	{
		return(false);
	}
	debug(" Sum: 0x%x.\n", config_sum);
	
	//Read config data.
	if (!read_flash((fsec * FLASH_SECTOR_SIZE) + foff + config_offset,
					param, len))
	{
		return(false);
	}
	
	//Calculate sum
	sum = calc_chksum(((uint8_t *)(param)), len);
	if (sum != config_sum)
	{
		error("Configuration data is corrupted (sum 0x%x, expected sum 0x%x).\n", sum, config_sum);
		return(false);
	}
	
	return(true);
}

struct config *read_cfg_flash(void)
{
	struct config *cfg;
	
	debug("Loading configuration from 0x%x.\n", CONFIG_FLASH_ADDR);
	
	cfg = db_malloc(sizeof(struct config), "cfg read_cfg_flash");
	if (!cfg)
	{
		error("Could not allocate memory for configuration.\n");
		return(NULL);
	}
	if (!sdk_system_param_load(CONFIG_FLASH_ADDR, 0, cfg, sizeof(struct config)))
	{
		error("Could not load configuration.\n");
		goto error;
	}
	if (cfg->signature != ESP_CONFIG_SIG)
	{
		error("Wrong configuration signature 0x%x should be 0x%x.\n",
			  cfg->signature, ESP_CONFIG_SIG);
		goto error;
	}
	if ((cfg->bver != CONFIG_MAJOR_VERSION) || (cfg->cver < CONFIG_MINOR_VERSION))
	{
		error("Wrong configuration data version %d.%d expected %d.%d.\n",
		      cfg->bver, cfg->cver, CONFIG_MAJOR_VERSION,
		      CONFIG_MINOR_VERSION);
		goto error;
	}
	else if (cfg->cver > CONFIG_MINOR_VERSION)
	{
		warn("Wrong, but working, configuration data version %d.%d expected %d.%d.\n",
		      cfg->bver, cfg->cver, CONFIG_MAJOR_VERSION,
		      CONFIG_MINOR_VERSION);
	}
	//Fix the hostname address.
	//(char *)cfg->hostname = (cfg->network_mode + 1);
	return(cfg);
	
error:
	db_free(cfg);
	return(NULL);
}

/**
 * @brief Save configuration data to flash.
 * 
 * Reimplementation of a function available in a later SDK.
 * 
 * @param fsec Sector offset in flash.
 * @param param Pointer to data buffer.
 * @param len Length of data to write.
 */
static bool sdk_system_param_save_with_protect(uint16_t fsec, void *param, uint16_t len)
{
	//Data for selecting the right copy.
	uint32_t config_info[2];
	
	debug("Writing configuration to flash.\n");
	if (len & 0x03)
	{
		error("Unaligned length.\n");
		return(false);
	}	

	//Read copy offset from the third sector.
	if (!read_flash((fsec + 2) * FLASH_SECTOR_SIZE, config_info,
					sizeof(uint32_t)))
	{
		return(false);
	}
	
	//Calculate sum
	config_info[1] = calc_chksum(((uint8_t *)(param)), len);
	
	if (config_info[0])
	{
		config_info[0] = 0;
	}
	else
	{
		config_info[0] = 4096;
	}
	
	debug(" Offset: 0x%x.\n", config_info[0]);
	debug(" Sum: 0x%x.\n", config_info[1]);
	//Write config sector offset and sum to the third sector.
	if (!write_flash((fsec + 2) * FLASH_SECTOR_SIZE, config_info, sizeof(uint32_t) * 2))
	{
		return(false);
	}

	//Write config data.
	if (!write_flash((fsec * FLASH_SECTOR_SIZE) + config_info[0], (uint32_t *)param, len))
	{
		return(false);
	}
	
	return(true);
}

bool write_cfg_flash(struct config cfg)
{
	debug("Saving configuration at 0x%x.\n", CONFIG_FLASH_ADDR);
	
	debug(" Setting configuration signature 0x%x.\n", ESP_CONFIG_SIG);
	cfg.signature = ESP_CONFIG_SIG;
	cfg.bver = CONFIG_MAJOR_VERSION;
	cfg.cver = CONFIG_MINOR_VERSION;
	return(sdk_system_param_save_with_protect(CONFIG_FLASH_ADDR, &cfg, sizeof(struct config)));
}
