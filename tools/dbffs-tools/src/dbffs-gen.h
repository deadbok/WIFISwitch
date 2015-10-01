/** 
 * @file dbffs-gen.h
 *
 * @brief Top level routines to generate a DBF file system image.
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
#ifndef DBFFS_GEN_H
#define DBFFS_GEN_H

#include <ftw.h> //ftw
#include <stdint.h> //Fixed width integer types.
#include <stdbool.h> //Bool.

/**
 * @brief Number of entries in the file system.
 */
extern unsigned short fs_n_entries;
/**
 * @brief List of entries in the file system.
 */
extern void *fs_entries;
/**
 * @brief The current entry in the file system.
 */
extern void *current_fs_entry;

/**
 * @brief Convert from host byte order to esp8266 byte order, 16 bit.
 * 
 * @param v Value to convert.
 * @return Swapped value.
 */
extern uint16_t swap16(uint16_t v);
/**
 * @brief Convert from host byte order to esp8266 byte order, 32 bit.
 * 
 * @param v Value to convert.
 * @return Swapped value.
 */
extern uint32_t swap32(uint32_t v);

/**
 * @brief Handle file system entries within a path.
 * 
 * ``nftw`` callback.
 * 
 * @note See ``nftw`` in the POSIX standard.
 * 
 * @param path Path of the current entry.
 * @param pstat Pointer to stat info for the entry.
 * @param flag ``nftw`` flags of the entry.
 * @param pftw Pointer to an FTW struct with members ``base`` as offset of entry in path, and ``level`` as path depth.
 * @return 0 to keep going, some other value to stop.
 */
extern int handle_entry(const char *path,
						const struct stat *pstat,
						int flag,
						struct FTW *pftw);
#endif //DBFFS_GEN_H
