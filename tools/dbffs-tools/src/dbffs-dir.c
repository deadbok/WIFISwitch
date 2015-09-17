/** 
 * @file dbffs-dir.c
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

struct dbffs_dir_hdr *create_dir_entry(const char *path, char *entryname)
{
	char *dir;
	size_t dir_len;
	struct dbffs_dir_hdr *entry;
	
	printf("-> dir\n");
	errno = 0;
	dir_len = strlen(path);
	dir = malloc(dir_len + 2);
	if (!dir)
	{
		die("Could not allocate memory for directory name.");
	}
	strcpy(dir, path);
	strcat(dir, "/");
	strcat(dir, "\0");
	//Get mem and start populating a dir entry.
	errno = 0;
	entry = calloc(sizeof(struct dbffs_dir_hdr), sizeof(uint8_t));
	if (!entry || (errno > 0))
	{
		die("Could not allocate memory for directory entry.");
	}
	entry->signature = DBFFS_DIR_SIG;
	entry->name = entryname;
	entry->name_len  = strlen(entryname);
	entry->entries = populate_fs_image(dir);
	printf("<- %s %d entries.\n", path, entry->entries);
	
	return(entry);
}

void write_dir_entry(const struct dbffs_dir_hdr *entry, FILE *fp)
{
	size_t ret;
	uint16_t word;
	uint32_t dword;
	
	//Write signature.
	errno = 0;
	dword = swap32(entry->signature);
	ret = fwrite(&dword, sizeof(uint8_t), sizeof(entry->signature), fp);
	if ((ret != sizeof(entry->signature)) || (errno > 0))
	{
		die("Could not write directory entry signature.");
	}
	//Calculate offset of the next entry.
	dword = swap32(sizeof(entry->signature) + 4 +
					sizeof(entry->name_len) +
					entry->name_len + sizeof(entry->entries));
	//Write offset to next entry.
	errno = 0;
	ret = fwrite(&dword, sizeof(uint8_t), sizeof(dword), fp);
	if ((ret != sizeof(dword)) || (errno > 0))
	{
		die("Could not write offset to next entry.");
	}
	//Write name length.
	errno = 0;
	ret = fwrite(&entry->name_len, sizeof(uint8_t), sizeof(entry->name_len), fp);
	if ((ret != sizeof(entry->name_len)) || (errno > 0))
	{
		die("Could not write directory name length.");
	}
	//Write name.
	errno = 0;
	ret = fwrite(entry->name, sizeof(uint8_t), entry->name_len, fp);
	if ((ret != entry->name_len) || (errno > 0))
	{
		die("Could not write directory name.");
	}
	//Write number of entries.
	errno = 0;
	word = swap16(entry->entries);
	ret = fwrite(&word, sizeof(uint8_t), sizeof(entry->entries), fp);
	if ((ret != sizeof(entry->entries)) || (errno > 0))
	{
		die("Could not write directory entries.");
	}
}
