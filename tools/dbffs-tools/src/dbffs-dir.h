/** 
 * @file dbffs-dir.h
 *
 * @brief Routines for handling directory entries.
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
#ifndef DBFFS_DIR_H
#define DBFFS_DIR_H

#include "dbffs.h"

/**
 * @brief Create a directory entry.
 *
 * @param path Path to the entry in the root directory to use as source.
 * @param entryname The name of the entry to add to the file system.
 * @return A pointer to a directory entry.
 */
extern struct dbffs_dir_hdr *create_dir_entry(const char *path, char *entryname);
/**
 * @brief Write a dir entry to a file.
 * 
 * @param entry Dir entry pointer.
 * @param fp Output file pointer.
 */
extern void write_dir_entry(const struct dbffs_dir_hdr *entry, FILE *fp);


#endif //DBFFS_DIR_H
