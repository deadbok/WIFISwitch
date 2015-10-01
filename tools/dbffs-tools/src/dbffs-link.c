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
/**
 * @brief Use X/Open 5, incorporating POSIX 1995 (for nftw).
 */
#define _XOPEN_SOURCE 500 //For nftw.
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

struct dbffs_link_hdr *create_link_entry(const char *entryname, const char *target)
{
	struct dbffs_link_hdr *entry;
	
	//Get mem and start populating a dir entry.
	errno = 0;
	entry = calloc(sizeof(struct dbffs_link_hdr), sizeof(uint8_t));
	if (!entry || (errno > 0))
	{
		die("Could not allocate memory for link entry.");
	}
	entry->signature = DBFFS_LINK_SIG;
	if (!(entry->name = calloc(strlen(entryname) + 1, sizeof(char))))
	{
		die("Could not allocate memory for link entry name.");
	}
	strcpy(entry->name, entryname);
	entry->name_len  = strlen(entryname);
	if (!(entry->target = calloc(strlen(target) + 1, sizeof(char))))
	{
		die("Could not allocate memory for link entry target.");
	}
	strcpy(entry->target, target);
	info("  Creating link %s -> %s.\n", entry->name, entry->target);
	entry->target_len = strlen(target);

	return(entry);
}

uint32_t write_link_entry(const struct dbffs_link_hdr *entry, FILE *fp)
{
	size_t ret;
	uint32_t dword;
	
	//Write signature.
	errno = 0;
	ret = fwrite(&entry->signature, sizeof(uint8_t), sizeof(entry->signature), fp);
	if ((ret != sizeof(entry->signature)) || (errno > 0))
	{
		die("Could not write link entry signature.");
	}
	//Calculate offset of the next entry.
	if (entry->next)
	{
		dword = sizeof(entry->signature) + sizeof(dword) +
				sizeof(entry->name_len) + entry->name_len +
				sizeof(entry->target_len) + entry->target_len;
	}
	else
	{
		dword = 0;
	}

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
	return(dword);
}
