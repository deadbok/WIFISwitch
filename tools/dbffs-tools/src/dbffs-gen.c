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
#include "dbffs-dir.h"
#include "dbffs-file.h"
#include "dbffs-link.h"

unsigned short fs_n_entries = 0;
void *fs_entries = NULL;
void *current_fs_entry = NULL;

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

void create_entry(const char *path, char *entryname)
{
	struct stat statbuf;
	size_t dir_len;
	char *dir;
	void *ret;

	if (fs_n_entries >= DBFFS_MAX_ENTRIES)
	{
		die("No more entries in file system.");
	}
	errno = 0;
	//Use lstat to find file type. Readdir could do this, but not on all
	//systems.
	if (lstat(path, &statbuf) != 0)
	{
		die("Error getting file info.");
	}
	switch (S_IFMT & statbuf.st_mode)
	{
		case S_IFREG:
			ret = create_file_entry(path, entryname);
			//Add to the list.
			//By some strange coincidence I have put the same data in the top
			//part of every header struct, so just pick a random header type
			//and cast to that.
			((struct dbffs_file_hdr *)(current_fs_entry))->next = ret;
			current_fs_entry = ret;
			fs_n_entries++;
			break;
		case S_IFDIR:
			dir_len = strlen(path);
			dir = malloc(dir_len + 2);
			if (!dir)
			{
				die("Could not allocate memory for directory name.");
			}
			strcpy(dir, path);
			strcat(dir, "/");
			strcat(dir, "\0");
			//Add dir.
			printf("-> %s, ", dir);
			ret = create_dir_entry(dir, entryname);
			((struct dbffs_dir_hdr *)(current_fs_entry))->next = ret;
			current_fs_entry = ret;
			fs_n_entries++;
			//Add entries in dir.
			populate_fs_image(dir);
			printf("<- %s %d entries.\n", dir, ((struct dbffs_dir_hdr *)(ret))->entries);
			free(dir);
			break;
		case S_IFLNK:
			ret = create_link_entry(path, entryname);
			//Add to the list.
			((struct dbffs_link_hdr *)(current_fs_entry))->next = ret;
			current_fs_entry = ret;
			fs_n_entries++;
			break;
		default:
			printf("unsupported type, skipping.\n");
	}
}

unsigned int populate_fs_image(const char* root_dir)
{
	DIR *dir;
	char *filename;
	char *entryname;
    struct dirent *dp;
    size_t filename_len;
    size_t root_dir_len;
    unsigned int entries = 0;
    
    errno = 0;
    if (!(dir = opendir(root_dir)))
    {
        die("Cannot open directory.");
    }
    errno = 0;
    if (!(dp = readdir(dir)))
    {
        die("Cannot read directory\n");
	}
	
	while (dp)
	{
		//Skip "." and "..", also skips dot files, stone cold.
		if ((strncmp(dp->d_name, ".", 1) != 0))
		{
			filename_len = strlen(dp->d_name);
			root_dir_len = strlen(root_dir);
			
			errno = 0;
			filename = malloc(root_dir_len + filename_len + 1);
			if (errno > 0)
			{
				die("Could not allocate memory for new path.");
			}
			strcpy(filename, root_dir);
			strcat(filename, dp->d_name);
			filename[root_dir_len + filename_len] = '\0';
			entryname = malloc(filename_len + 1);
			if (errno > 0)
			{
				die("Could not allocate memory for entry name.");
			}
			strcpy(entryname, dp->d_name);
			printf("%s, ", filename);
			create_entry(filename, entryname);
			entries++;
			free(filename);
		}
		errno = 0;
		dp = readdir(dir);
		if (errno > 0)
		{
			die("Could not read directory contents.");
		}
	}
	errno = 0;
	closedir(dir);
	if (errno > 0)
	{
		die("Could not close directory.");
	}
	return(entries);
}

unsigned short count_dir_entries(const char* root_dir)
{
	DIR *dir;
    struct dirent *dp;
    unsigned int ret = 0;
    
    errno = 0;
    if (!(dir = opendir(root_dir)))
    {
        die("Cannot open directory.");
    }
    errno = 0;
    if (!(dp = readdir(dir)))
    {
        die("Cannot read directory\n");
	}
	
	while (dp)
	{
		//Skip "." and "..", also skips dot files, stone cold.
		if ((strncmp(dp->d_name, ".", 1) != 0))
		{
			ret++;
			if (ret > 65535)
			{
				die("More than 65536 entries in diectory.");
			}
		}
		errno = 0;
		dp = readdir(dir);
		if (errno > 0)
		{
			die("Could not read directory contents.");
		}
	}
	errno = 0;
	closedir(dir);
	if (errno > 0)
	{
		die("Could not close directory.");
	}
	return((unsigned short)ret);
}
