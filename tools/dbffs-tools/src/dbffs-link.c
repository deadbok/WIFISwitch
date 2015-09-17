/** 
 * @file dbffs-link.c
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
#include <errno.h> //errno
#include <stdio.h> //printf
#include <string.h>
#include <stddef.h> //size_t
#include <stdint.h> //Fixed width integer types.
#include <stdlib.h> //malloc
#include <unistd.h> //getopt
#include "common.h"
#include "dbffs.h"
#include "dbffs-gen.h"
#include "dbffs-file.h"

/**
 * @brief Get the contents of a symbolic link.
 * 
 * @param path Path to the link.
 * @return Link target path or NULL.
 */
static char *db_readlink(const char *path)
{
    int ret;
    char *link;
    int link_size = 33;
    
    //Get initial mem.
	errno = 0;
	link = malloc(link_size);
	if (!link || (errno > 0))
	{
		die("Could not allocate memory for link target.");
	}    

	//Stop trying before memory reallocation runs havoc.
	while (link_size < 65536)
	{
		errno = 0;
		ret = readlink(path, link, link_size - 1);
		if ((ret < 1) || (errno > 1))
		{
			die("Error getting symbolic link target.");
		}
		if (ret < link_size) 
		{
			link[ret] = '\0';
			return(link);
		}
		//Not enough mem, get some more.
		link_size += 32;
		errno = 0;
		if ((realloc(link, link_size) == NULL) || (errno > 0))
		{
			die("Could not expand memory for link target.");
		}
	}
    return(NULL);
}

struct dbffs_link_hdr *create_link_entry(const char *path, char *entryname)
{
	struct dbffs_link_hdr *entry;
	char *link_target;
	
	link_target = db_readlink(path);
	if (!link_target)
	{
		die("Could not get the link target.");
	}
    printf("-> %s, link\n", link_target);
	//Get mem and start populating a dir entry.
	errno = 0;
	entry = calloc(sizeof(struct dbffs_link_hdr), sizeof(uint8_t));
	if (!entry || (errno > 0))
	{
		die("Could not allocate memory for link entry.");
	}
	entry->signature = DBFFS_LINK_SIG;
	entry->name = entryname;
	entry->name_len  = strlen(entryname);
	entry->target = link_target;

	return(entry);
}

void write_link_entry(const struct dbffs_link_hdr *entry, FILE *fp)
{
	size_t ret;
	uint32_t dword;
	
	//Write signature.
	errno = 0;
	dword = swap32(entry->signature);
	ret = fwrite(&dword, sizeof(uint8_t), sizeof(entry->signature), fp);
	if ((ret != sizeof(entry->signature)) || (errno > 0))
	{
		die("Could not write link entry signature.");
	}
	//Calculate offset of the next entry.
	dword = swap32(sizeof(entry->signature) + 4 +
					sizeof(entry->name_len) +
					entry->name_len + sizeof(dword));
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
		die("Could not write name length.");
	}
	//Write name.
	errno = 0;
	ret = fwrite(entry->name, sizeof(uint8_t), entry->name_len, fp);
	if ((ret != entry->name_len) || (errno > 0))
	{
		die("Could not write name.");
	}
	//Write target path length.
	errno = 0;
	ret = fwrite(&entry->target_len, sizeof(uint8_t), sizeof(entry->target_len), fp);
	if ((ret != sizeof(entry->target_len)) || (errno > 0))
	{
		die("Could not write target path length.");
	}
	//Write target path.
	errno = 0;
	ret = fwrite(entry->target, sizeof(uint8_t), entry->target_len, fp);
	if ((ret != entry->target_len) || (errno > 0))
	{
		die("Could not write target name.");
	}
}
