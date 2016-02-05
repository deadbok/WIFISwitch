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
#include "FreeRTOS.h"
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "main.h"
#include "debug.h"
#include "config/config.h"
#include "fs/int_flash.h"

/**
 * @brief Offset to determine if we are using copy 1 or 2.
 */
static uint32_t config_offset = 0;
/**
 * @brief Checksum for config data.
 */
static uint32_t config_sum;

/**
 * @brief Flash read wrapper for debug and error handling.
 */
static bool read_flash(uint32_t foff, void *data, uint16_t len)
{
	uint8_t ret;
	
	debug("Reading from flash at: 0x%x (%d byte(s)).\n", foff, len);
	 
	ret = sdk_spi_flash_read(foff, data, len);
	if (ret)
	{
		error("Error reading flash: %d.\n", ret);
		return(false);
	}
	return(true);
}

/**
 * @brief Flash write wrapper for debug and error handling.
 */
static bool write_flash(uint32_t foff, void *data, uint16_t len)
{
	uint8_t ret;
	
	debug("Write from flash at: 0x%x (%d byte(s)).\n", foff, len);
	 
	ret = sdk_spi_flash_read(foff, data, len);
	if (ret)
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
	uint16_t i;
	uint32_t sum = 0;
	
	debug("Reading configuration from flash.\n");
	if (foff & 0x03)
	{
		error("Unaligned address.\n");
		return(false);
	}
	if (len & 0x03)
	{
		error("Unaligned length.\n");
		return(false);
	}	

	//Read copy offset and sum from the third sector.
	if (!read_flash((fsec + 2) * FLASH_SECTOR_SIZE, &config_offset, sizeof(uint32_t)))
	{
		return(false);
	}
	debug("Configuration offset: %d.\n", config_offset);

	if (!read_flash((fsec + 2) * FLASH_SECTOR_SIZE + sizeof(uint32_t), &config_sum, sizeof(uint32_t)))
	{
		return(false);
	}
	
	//Read config data.
	if (!read_flash((fsec * FLASH_SECTOR_SIZE) + foff + config_offset, (uint32_t *)param, len))
	{
		return(false);
	}
	
	//Calculate sum
	for (i = 0; i < len; i++)
	{
		sum += ((uint8_t *)(param))[i];
	}
	if (sum != config_sum)
	{
		error("Configuration data is corrupted.\n");
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
		db_free(cfg);
		return(NULL);
	}
	if (cfg->signature != ESP_CONFIG_SIG)
	{
		error("Wrong configuration signature 0x%x should be 0x%x.\n",
			  cfg->signature, ESP_CONFIG_SIG);
		return(NULL);
	}
	if ((cfg->bver != CONFIG_MAJOR_VERSION) || (cfg->cver < CONFIG_MINOR_VERSION))
	{
		error("Wrong configuration data version %d.%d expected %d.%d.\n",
		      cfg->bver, cfg->cver, CONFIG_MAJOR_VERSION,
		      CONFIG_MINOR_VERSION);
		      return(NULL);
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
	uint16_t i;
	
	debug("Writing configuration to flash.\n");
	if (len & 0x03)
	{
		error("Unaligned length.\n");
		return(false);
	}	

	//Calculate sum
	for (i = 0; i < len; i++)
	{
		config_sum += ((uint8_t *)(param))[i];
	}
	
	if (config_offset)
	{
		config_offset = 0;
	}
	else
	{
		config_offset = 4096;
	}
	debug(" Offset 0x%x.\n", config_offset);
	
	//Write copy offset and sum to the third sector.
	if (!write_flash((fsec + 2) * FLASH_SECTOR_SIZE, &config_offset, sizeof(uint32_t)))
	{
		return(false);
	}
	
	if (!write_flash((fsec + 2) * FLASH_SECTOR_SIZE + sizeof(uint32_t), &config_sum, sizeof(uint32_t)))
	{
		return(false);
	}
	
	//Write config data.
	if (!write_flash(fsec * FLASH_SECTOR_SIZE + config_offset, (uint32_t *)param, len))
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
