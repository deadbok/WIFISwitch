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
#include "user_interface.h"
#include "config.h"

struct config ICACHE_FLASH_ATTR *read_cfg_flash(void)
{
	struct config *cfg;
	
	debug("Loading configuration from 0x%x.\n", CONFIG_FLASH_ADDR);
	
	cfg = db_malloc(sizeof(struct config), "cfg read_cfg_flash");
	if (!cfg)
	{
		error("Could not allocate memory for configuration.\n");
		return(NULL);
	}
	if (!system_param_load(CONFIG_FLASH_ADDR, 0, cfg, sizeof(struct config)))
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
	return(cfg);
}

bool ICACHE_FLASH_ATTR write_cfg_flash(struct config cfg)
{
	debug("Saving configuration at 0x%x.\n", CONFIG_FLASH_ADDR);
	
	debug(" Setting configuration signature 0x%x.\n", ESP_CONFIG_SIG);
	cfg.signature = ESP_CONFIG_SIG;
	cfg.bver = CONFIG_MAJOR_VERSION;
	cfg.cver = CONFIG_MINOR_VERSION;
	return(system_param_save_with_protect(CONFIG_FLASH_ADDR, &cfg, sizeof(struct config)));
}
