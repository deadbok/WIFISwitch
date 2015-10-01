/** 
 * @file dbffs-file.h
 *
 * @brief File related DBFFS routines.
 * 
 * *This tool is meant for small file systems used on embedded
 * systems, and keeps everything in memory while building the
 * image.*
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
#ifndef DBFFS_FILE_H
#define DBFFS_FILE_H

#include "dbffs.h"

/**
 * @brief Create a file entry.
 *
 * @param path Path to the entry in the root directory to use as source.
 * @param entryname The name of the entry to add to the file system.
 * @return Pointer to the directory entry.
 */
extern struct dbffs_file_hdr *create_file_entry(const char *path, const char *entryname);
/**
 * @brief Write a file entry to a file.
 * 
 * @param entry File entry pointer.
 * @param fp Output file pointer.
 */
extern uint32_t write_file_entry(const struct dbffs_file_hdr *entry, FILE *fp);

#endif //DBFFS_FILE_H
