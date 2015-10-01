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
/**
 * @brief Use X/Open 5, incorporating POSIX 1995 (for nftw).
 */
#define _XOPEN_SOURCE 500 //For nftw.

#include <errno.h> //errno
#include <stdio.h> //printf
#include <string.h>
#include <stddef.h> //size_t
#include <stdlib.h> //malloc
#include <unistd.h> //getopt
#include <ftw.h> //nftw
#include "common.h"
#include "dbffs.h"
#include "dbffs-gen.h"
#include "dbffs-file.h"
#include "dbffs-link.h"

/**
 * @brief Program version.
 */
#define DBBFS_IMAGE_VERSION "0.2.0"

char *root_dir = NULL;
bool verbose = false;

/**
 * @brief Print welcome message.
 */
static void print_welcome(void)
{
	printf("dbf file system image generation tool version " DBBFS_IMAGE_VERSION ".\n");
	printf("DBFFS version " DBFFS_VERSION "\n\n");
}

/**
 * @brief Print command line help.
 */
static void print_commandline_help(const char *progname)
{
	printf("Usage: %s [options] root_dir image_file\n", progname);
	printf("Create DBFFS image, image_file,  from files in root_dir.\n");	
	printf("Options:\n");
	printf(" -v: Be verbose.\n");
}

/**
 * @brief Main program.
 */
int main(int argc, char *argv[])
{
	int arg = 1;
	FILE *fp;
	void *fs_entry;
	uint32_t offset;
	unsigned int i = 0;
	char *image_filename = NULL;
	uint32_t fs_sig = DBFFS_FS_SIG;
    
	print_welcome();
	
	if (argc < 3)
	{
		print_commandline_help(argv[0]);
		die("Could not parse command line.");
	}
	
	if (strcmp(argv[arg], "-v") == 0)
	{
		if (argc != 4)
		{
			printf("Found %d command line arguments.\n", argc);
			print_commandline_help(argv[0]);
			die("Missing command line arguments.");			
		}
		verbose = true;
		arg++;
	}
		
	root_dir = argv[arg];
	arg++;
	image_filename = argv[arg];
	
	printf("Creating image in RAM from files in %s.\n", root_dir);
	//Scan source.
	nftw(root_dir, handle_entry, 10, FTW_PHYS);

	//Write out the data structures to an image.
	printf("\nWriting image to file %s.\n", image_filename);
	errno = 0;
	fp = fopen(image_filename, "w");
	if (!fp || (errno > 0))
	{
		die("Could not open image file.");
	}
	//Write file system signature.
	errno = 0;
	if ((fwrite(&fs_sig, sizeof(uint8_t), sizeof(fs_sig), fp) != sizeof(fs_sig)) || (errno > 0))
	{
		die("Could not write file entry signature.");
	}
	for (fs_entry = fs_entries; fs_entry != NULL;
		 fs_entry = ((struct dbffs_file_hdr *)(fs_entry))->next)
	{
			switch (*((uint32_t *)(fs_entry)))
			{
				case DBFFS_FILE_SIG:
					printf(" Writing file %s.\n", ((struct dbffs_file_hdr *)(fs_entry))->name);
					offset = write_file_entry((struct dbffs_file_hdr *)(fs_entry), fp);
					break;
				case DBFFS_LINK_SIG:
					printf(" Writing link %s -> %s.\n", ((struct dbffs_link_hdr *)(fs_entry))->name, ((struct dbffs_link_hdr *)(fs_entry))->target);
					offset = write_link_entry((struct dbffs_link_hdr *)(fs_entry), fp);
					break;
				default:
					printf(" Unknown signature 0x%x.\n", ((struct dbffs_file_hdr *)(fs_entry))->signature);
					break;
			}
			info(" Next header at 0x%x.\n", offset);
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
