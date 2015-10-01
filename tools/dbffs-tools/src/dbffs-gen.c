/** 
 * @file dbffs-gen.c
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
/**
 * @brief Use X/Open 5, incorporating POSIX 1995 (for nftw).
 */
#define _XOPEN_SOURCE 500 //For nftw.

#include <sys/stat.h>
#include <string.h>
#include <stddef.h> //size_t
#include <stdint.h> //Fixed width integer types.
#include <stdlib.h> //malloc
#include <unistd.h> //getopt
#include <errno.h> //errno
#include <stdio.h> //printf
#include "common.h"
#include "dbffs.h"
#include "dbffs-gen.h"
#include "dbffs-file.h"
#include "dbffs-link.h"

unsigned short fs_n_entries = 0;
void *current_fs_entry = NULL;
void *fs_entries = NULL;

/**
 * @brief Current path in the target image.
 */ 
static char fs_path[DBFFS_MAX_PATH_LENGTH];
/**
 * @brief Current root when handling links.
 */
static char *current_root;
/**
 * @brief True of the current link target is a directory..
 */
static bool linked_dir;

uint16_t swap16(uint16_t v)
{
	uint8_t ret[2];
	
	ret[0] = v >> 8;
	ret[1] = v;
	
	return (*((uint16_t *)ret));
}

uint32_t swap32(uint32_t v)
{
	uint8_t ret[4];
	
	ret[0] = v >> 24;
	ret[1] = v >> 16;
	ret[2] = v >> 8;
	ret[3] = v;
	
	return (*((uint32_t *)ret));
}

/**
 * @brief Add an entry to the file system entry list.
 * 
 * @param entry Pointer to the entry to add.
 */
static void add_fs_entry(void *entry)
{
	if (current_fs_entry)
	{
		((struct dbffs_file_hdr *)(current_fs_entry))->next = entry;
	}
	else
	{
		fs_entries = entry;
	}
	current_fs_entry = entry;
	fs_n_entries++;
}

/**
 * @brief Handle entries within a link.
 * 
 * ``nftw`` callback.
 * 
 * @note See ``nftw`` the in POSIX standard.
 * 
 * @param path Path of the current entry.
 * @param pstat Pointer to stat info for the entry.
 * @param flag ``nftw`` flags of the entry.
 * @param pftw Pointer to an FTW struct with members ``base`` as offset of entry in path, and ``level`` as path depth.
 * @return 0 to keep going, some other value to stop.
 */
static int handle_link_entry(const char *path, const struct stat *pstat, int flag, struct FTW *pftw)
{
	void *ret;
	size_t size = 0;
	char *target_path;
	char *fs_path_pos;
	struct stat statbuf;
	char fixed_target_path[2048];

	fs_path_pos = fs_path + strlen(fs_path);
	switch (flag)
	{
		case FTW_F:
			if (linked_dir)
			{
				strcat(fs_path, "/");
				strcat(fs_path, path + pftw->base);
			}
			info("  Linked file %s -> %s.\n", fs_path, path);
			//Make path absolute to link target.
			target_path = rpath(path + strlen(root_dir) - 1, NULL);
			ret = create_link_entry(fs_path, target_path);
			add_fs_entry(ret);
			free(target_path);
			*fs_path_pos = '\0';
			break;
		case FTW_D:
			//Check if we are till handling the linked dir.
			if (strcmp(current_root, path) != 0)
			{
				strcat(fs_path, "/");
				strcat(fs_path, path + pftw->base);
				info("  Linked directory %s -> %s.\n", path, fs_path);
				linked_dir = false;
			}
			break;
		case FTW_SL:
			errno = 0;
			if (stat(path, &statbuf) == -1)
			{
				die("Could not read link target information.\n");
			}
			switch (statbuf.st_mode & S_IFMT)
			{
				case S_IFDIR:
					info("  Link target is a directory.\n");
					linked_dir = true;
					strcpy(fs_path, path + strlen(root_dir) - 1);
					break;
				case S_IFLNK:
					info("  Link target is a link.\n");
					linked_dir = false;
					break;
				case S_IFREG:
					info("  Link target is a file.\n");
					linked_dir = false;
					strcpy(fs_path, path + strlen(root_dir) - 1);
					break;
				default:
					info("  Target type unknown %d.\n", statbuf.st_mode & S_IFMT);
					break;
			}
			info("  Link %s -> %s.\n", path, fs_path);
			//Copy current path without current entry.
			memcpy(fixed_target_path, path, pftw->base);
			//Add path to the link target.
			size = readlink(path, fixed_target_path + pftw->base, 2048 - pftw->base);
			if (size < 0)
			{
				perror("Error reading link target.");
				return(1);
			}
			//Terminate string, now containing the path to the link target.
			fixed_target_path[pftw->base + size] = '\0';
			info(" Link target %s.\n", fixed_target_path);
			current_root = fixed_target_path;
			nftw(fixed_target_path, handle_link_entry, 10, FTW_PHYS);
			break;
		default:
			info("Unsupported type %d in link, skipping %s -> %s.\n", flag, path, fs_path);
	}
	return(0);
}
	
int handle_entry(const char *path, const struct stat *pstat, int flag, struct FTW *pftw)
{
	void *ret;
	size_t size;
	struct stat statbuf;
	char target_path[2048];
	
	switch (flag)
	{
		case FTW_F:
			strcpy(fs_path, path + strlen(root_dir) - 1);
			info(" File %s -> %s.\n", path, fs_path);
			ret = create_file_entry(path, fs_path);
			add_fs_entry(ret);
			fs_path[pftw->base - strlen(root_dir)] = '\0';
			break;
		case FTW_D:
			strcpy(fs_path, path + strlen(root_dir) - 1);
			info(" Directory %s -> %s.\n", path, fs_path);
			break;
		case FTW_SL:
			errno = 0;
			if (stat(path, &statbuf) == -1)
			{
				die("Could not read link target information.\n");
			}
			switch (statbuf.st_mode & S_IFMT)
			{
				case S_IFDIR:
					info(" Link target is a directory.\n");
					linked_dir = true;
					strcpy(fs_path, path + strlen(root_dir) - 1);
					break;
				case S_IFLNK:
					info(" Link target is a link.\n");
					linked_dir = false;
					break;
				case S_IFREG:
					info(" Link target is a file.\n");
					linked_dir = false;
					strcpy(fs_path, path + strlen(root_dir) - 1);
					break;
				default:
					info(" Target type unknown %d.\n", statbuf.st_mode & S_IFMT);
					break;
			}
			info(" Link %s -> %s.\n", path, fs_path);
			//Copy current path without current entry.
			memcpy(target_path, path, pftw->base);
			//Add path to the link target.
			size = readlink(path, target_path + pftw->base, 2048 - pftw->base);
			if (size < 0)
			{
				perror("Error reading link target.");
				return(1);
			}
			//Terminate string, now containing the path to the link target.
			target_path[pftw->base + size] = '\0';
			info(" Link target %s.\n", target_path);
			current_root = target_path;
			nftw(target_path, handle_link_entry, 10, FTW_PHYS);
			break;
		default:
			info("Unsupported type %d, skipping %s -> %s.\n", flag, path, fs_path);
	}
	return(0);
}

