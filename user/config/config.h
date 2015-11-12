/**
 * @file config.h
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
 
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h> //Fixed width integer types.

/**
 * @brief Signature to tell if the configuration data sound.
 */
#ifndef ESP_CONFIG_SIG
	#error ESP_CONFIG_SIG configuration signature needs to be defined.
#endif

/**
 * @brief Major configuration data version.
 * 
 * *Change this if new version breaks backward compatibility.*
 */
#define CONFIG_MAJOR_VERSION 1
/**
 * @brief Minor configuration data version.
 * 
 * *Change this if new version does not break backward compatibility.*
 */
#define CONFIG_MINOR_VERSION 1
/**
 * @brief Entries in config data including version and signature.
 */
#define CONFIG_ENTRIES 5
/**
 * @brief Configuration settings that are stored in flash goes in here.
 * 
 * *This is limited to 4Kb.*
 */
struct config
{
	/**
	 * @brief Unique configuration signature.
	 */
	uint32_t signature;
	/**
	 * @brief "Breaking" configuration data version.
	 * 
	 * *Change this if new version breaks backward compatibility.*
	 */
	uint16_t bver;
	/**
	 * @brief "Compatible" configuration data version.
	 * 
	 * *Change this if new version does not break backward compatibility.*
	 */
	uint16_t cver;
	/**
	 * @brief Address of the file system.
	 */
	uint32_t fs_addr;
	/**
	 * @brief Network mode.
	 */
	uint8_t network_mode;
#ifdef DB_ESP8266
	/**
	 * @brief Padding to align on 4 byte boundary.
	 */
	uint8_t padding[3];
#endif //DB_ESP8266
}  __attribute__ ((__packed__));

#ifdef DB_ESP8266
/**
 * @brief Read configuration parameters from flash.
 * 
 * @return Pointer to a ``struct config`` or NULL on error.
 */
extern struct config *read_cfg_flash(void);
/**
 * @brief Write configuration data to the flash.
 * 
 * @param cfg A ``struct config`` with configuration values.
 * @return ``true`` on success, ``false`` on error.
 */
extern bool write_cfg_flash(struct config cfg);
#else
/**
 * @brief Structure with size and description of a configuration entry.
 */
struct cfg_entry
{
	/**
	 * @brief Size of the entry in bytes.
	 * 
	 * *Zero means the entry is a string.*
	 */
	size_t size;
	/**
	 * @brief Type.
	 * 
	 * Values:
	 *  * 'c' a string.
	 *  * 's' a signed value.
	 *  * 'u' an unsigned value.
	 */
	char type; 
	/**
	 * @brief Parameter name.
	 */
	char *name;
	/**
	 * @brief Description of the config entry.
	 */
	char *info;
};

/**
 * @brief Array with description and size of each config entry.
 */
struct cfg_entry config_entries[CONFIG_ENTRIES] = {
	{4, 'u', "signature", "Configuration signature"},
	{2, 'u', "bver", "Breaking version"},
	{2, 'u', "cver", "Compatible version"},
	{4, 'u', "fs_addr", "Address in flash of the file system"},
	{5, 'u', "network_mode", "Use AP or client mode for network"}
};
#endif //DB_ESP8266

#endif //CONFIG_H
