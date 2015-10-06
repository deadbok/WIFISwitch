/** 
 * @file dbffs.h
 *
 * @brief ESP8266 DBFFS data structures and definitions.
 * 
 * Can only handle absolute links.
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
#ifndef DBFFS_H
#define DBFFS_H

#include <stdint.h>
#include "dbffs-std.h"

extern void init_dbffs(void);
extern void dbffs_free_file_header(struct dbffs_file_hdr *entry);
extern struct dbffs_file_hdr *dbffs_find_file_header(char *path);

/**
 * @brief Read data from an arbitrary position in the FS portion of the flash.
 * 
 * @param data Pointer to a buffer to place the data in.
 * @param read_addr Address to read from.
 * @param size Bytes to read.
 * @return True if everything went well, false otherwise.
 */
extern bool dbffs_read(const struct dbffs_file_hdr *file_hdr, void *data, unsigned int read_addr, size_t size);

#endif //DBFFS
