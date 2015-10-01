/** 
 * @file dbffs-link.h
 *
 * @brief Routines for handlink link entries.
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
#ifndef DBFFS_LINK_H
#define DBFFS_LINK_H

#include <errno.h> //errno
#include <stdio.h> //printf
#include <string.h>
#include <stddef.h> //size_t
#include <stdint.h> //Fixed width integer types.
#include <stdlib.h> //malloc
#include <unistd.h> //getopt
#include <dirent.h> //opendir, readir, closedir. dirent, DIR
#include <sys/stat.h> //lstat
#include "common.h"
#include "dbffs.h"
#include "dbffs-gen.h"
#include "dbffs-file.h"

/**
 * @brief Create a link entry.
 * 
 * @param entryname The name of the entry to add to the file system.
 * @param target The target of the link.
 * @return A pointer to a link entry.
 */
extern struct dbffs_link_hdr *create_link_entry(const char *entryname,
												const char *target);
/**
 * @brief Write a link entry to an image.
 *
 * @param entry Pointer to link entry to write to image.
 * @param fp Pointer to an open image file.
 */
extern uint32_t write_link_entry(const struct dbffs_link_hdr *entry, FILE *fp);

#endif //DBFFS_LINK_H
