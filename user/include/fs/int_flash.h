/**
 * @file int_flash.h
 *
 * @brief Routines for accessing the flash.
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
 
#ifndef INT_FLASH_H
#define INT_FLASH_H

#include <stdint.h> //Fixed width integer types.

/**
 *  @brief Highest address of the file system.
 */
#define MAX_FS_ADDR 0x2E000

/**
 * @brief Flash sector size.
 */
#define FLASH_SECTOR_SIZE 4096

/**
 * @brief Offset in the flash where the file system starts.
 * 
 * Offset into the flash, where the file system starts. The initial
 * value, is where the initialisation routine starts scanning for the
 * file system.
 */
extern size_t fs_addr;

//Flash access functions.
/**
 * @brief Return the flash chip's size, in bytes.
 * 
 * From "draco" on http://www.esp8266.com/viewtopic.php?f=13&t=2506
 * @return Size of flash in bytes.
 */
extern size_t flash_size(void);
/**
 * @brief Dump flash contents from an address until size bytes has been dumped.
 * 
 * Uses the Espressif API.
 * 
 * @param src_addr Start address.
 * @param size Number of bytes to dump.
 */
extern void flash_dump(unsigned int src_addr, size_t size);
/**
 * @brief Dump flash contents from an address until size bytes has been dumped.
 * 
 * Uses mapped access.
 * 
 * @param src_addr Start address.
 * @param size Number of bytes to dump.
 */
extern void flash_dump_mem(unsigned int src_addr, size_t size);
/**
 * @brief Read data from an arbitrary position in the FS portion of the flash.
 * 
 * @param data Pointer to a buffer to place the data in.
 * @param read_addr Address to read from.
 * @param size Bytes to read.
 * @return True if everything wen well, false otherwise.
 */
extern bool flash_aread(const void *data, unsigned int read_addr, size_t size);

#endif
