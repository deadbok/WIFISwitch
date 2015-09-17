/** 
 * @file dbffs-image.c
 *
 * @brief Generate a DBF file system image.
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
#include "dbffs-dir.h"
#include "dbffs-file.h"
#include "dbffs-link.h"

/**
 * @brief Program version.
 */
#define DBBFS_IMAGE_VERSION "0.0.2"

/**
 * @brief Print welcome message.
 */
static void print_welcome(void)
{
	printf("dbf file system image generation tool version " DBBFS_IMAGE_VERSION ".\n");
	printf("DBFFS version " DBBFS_IMAGE_VERSION "\n\n");
}

/**
 * @brief Print command line help.
 */
static void print_commandline_help(const char *progname)
{
	printf("Usage: %s ROOT IMAGE\n", progname);
	printf("Create DBFFS image, IMAGE, from filed in ROOT.\n");	
}

/**
 * @brief Main program.
 */
int main(int argc, char *argv[])
{
	char *root_dir = NULL;
	unsigned int root_entries = 0;
	unsigned int i = 0;
	char *image_filename = NULL;
	void *fs_entry;
	struct dbffs_dir_hdr *root_dir_entry;
	FILE *fp;
    
	print_welcome();
	
	if (argc < 2)
	{
		print_commandline_help(argv[0]);
		die("Error on command line");
	}
	root_dir = argv[1];
	image_filename = argv[2];
	
	printf("Creating image in RAM from files in %s.\n", root_dir);
	//Get mem and populate root dir entry.
	errno = 0;
	root_dir_entry = calloc(sizeof(struct dbffs_dir_hdr), sizeof(uint8_t));
	if ((root_dir_entry == NULL) || (errno > 0))
	{
		die("Could not allocate memory for root dir entry.");
	}
	root_dir_entry->signature = DBFFS_DIR_SIG;
	root_dir_entry->name = "/";
	root_dir_entry->name_len  = strlen(root_dir_entry->name);
	fs_entries = root_dir_entry;
	current_fs_entry = fs_entries;
	fs_n_entries++;
	
	//Scan source.
	root_entries = populate_fs_image(root_dir);

	root_dir_entry->entries = root_entries;
	printf("<- %s %d entries.\n", root_dir, root_entries);
	printf("Added a total of %d entries.\n", fs_n_entries);
	
	//Write out the data structures to an image.
	printf("\nWriting image to file %s.\n", image_filename);
	errno = 0;
	fp = fopen(image_filename, "w");
	if (!fp || (errno > 0))
	{
		die("Could not open image file.");
	}
	for (fs_entry = fs_entries; fs_entry != NULL;
		 fs_entry = ((struct dbffs_file_hdr *)(fs_entry))->next)
	{
			switch (*((uint32_t *)(fs_entry)))
			{
				case DBFFS_FILE_SIG:
					printf(" Writing file %s.\n", ((struct dbffs_dir_hdr *)(fs_entry))->name);
					write_file_entry(fs_entry, fp);
					break;
				case DBFFS_DIR_SIG:
					printf(" Writing directory %s.\n", ((struct dbffs_dir_hdr *)(fs_entry))->name);
					write_dir_entry(fs_entry, fp);
					break;
				case DBFFS_LINK_SIG:
					printf(" Writing link %s.\n", ((struct dbffs_dir_hdr *)(fs_entry))->name);
					write_link_entry(fs_entry, fp);
					break;
				default:
					printf(" Unknown signature 0x%x.\n", ((struct dbffs_dir_hdr *)(fs_entry))->signature);
					break;
			}
			i++;
	}
	errno = 0;
	fclose(fp);
	if (errno > 0)
	{
		die("Error closing image file.");
	}
	printf("%d entries written to image %s.\n", i, image_filename);
	return(EXIT_SUCCESS);
}
