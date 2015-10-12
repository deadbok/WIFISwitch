/** 
 * @file gen_config.c
 *
 * @brief Generate a binary image for flashing configuration data.
 * 
 * Creates a binary file that contain configuration data. This file is
 * then flashed to the ESP8266 and used by the firmware.
 * 
 * *Since this tool includes the configuration struct from the firmware
 * sources, this tool only works for the current firmware project."
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
#include <stdlib.h> //malloc
#include <unistd.h> //getopt
#include "config.h"
#include "common.h"

/**
 * @brief Program version.
 */
#define VERSION "0.0.1"

bool verbose = false;

/**
 * @brief Print welcome message.
 */
static void print_welcome(void)
{
	printf("ESP8266 configuration image generation tool version " VERSION ".\n");
	printf("COMPILED FOR: " PROJECT_NAME "\n\n");
}

/**
 * @brief Print command line help.
 */
static void print_commandline_help(const char *progname)
{
	unsigned int i;
	
	printf("Usage: %s [options] image_file", progname);
	//Skip signature and version, they are internal.
	for (i = 3; i < CONFIG_ENTRIES; i++)
	{
		printf(" %s", config_entries[i].name);
	}
	printf("\nCreate configuration image, image_file, writing the following options.\n");	
	printf("\nOptions:\n");
	printf(" -v: Be verbose.\n");
	
	printf("\nConfiguration parameters:\n");
	//Skip signature and version, they are internal.
	for (i = 3; i < CONFIG_ENTRIES; i++)
	{
		if (config_entries[i].size)
		{
			printf(" Variable \"%s\": %s.\n", config_entries[i].name, config_entries[i].info);
		}
		else
		{
			printf(" String \"%s\": %s.\n", config_entries[i].name, config_entries[i].info);
		}
	}
}

/**
 * @brief Parse a command line argument into a long long.
 * 
 * @param str String to parse.
 * @param entry Entry number in configuration structure.
 * @return long long representation of ``str``.
 */
static long long arg_parse_value(char *str, unsigned int entry)
{
	long long ret = 0;
	
	errno = 0;
	ret = strtoll(str, NULL, 0);
	if (errno > 0)
	{
		die("Could not convert configuration parameter.");
	}
    
    info("%s: %zd bytes \"%lld\" (%s).\n", config_entries[entry].info, config_entries[entry].size, ret, str);
	return(ret);
}

/**
 * @brief Parse a command line argument into a string.
 * 
 * @param str String to parse.
 * @param entry Entry number in configuration structure.
 * @return String..
 */
static char *arg_parse_string(char *str, unsigned int entry)
{
	//I know, I know, but it looks prettier as a function.
	info("%s: %zd bytes \"%s\".\n", config_entries[entry].info, config_entries[entry].size, str);
	return(str);
}

/**
 * @brief Main program.
 */
int main(int argc, char *argv[])
{
	FILE *fp;
	int arg = 1;
	struct config cfg;
	unsigned char sector;
	size_t config_size, i;
	unsigned int entry = 0;
	char *image_filename = NULL;
    
	print_welcome();
	
	//Fail on missing command line parameters.
	if (argc < 1 + (CONFIG_ENTRIES - 3))
	{
		print_commandline_help(argv[0]);
		die("Could not parse command line.");
	}
	
	//Fail on missing command line parameters with verbose option.
	if (strcmp(argv[arg], "-v") == 0)
	{
		if (argc !=  (3 + (CONFIG_ENTRIES - 3)))
		{
			printf("Found %d command line arguments.\n", argc);
			print_commandline_help(argv[0]);
			die("Wrong number of command line arguments.");			
		}
		verbose = true;
		arg++;
	}
	
	image_filename = argv[arg++];
	
	//Set configuration values.
	info("%s: 0x%x.\n", config_entries[0].info, ESP_CONFIG_SIG);
	cfg.signature = ESP_CONFIG_SIG;
	entry++;
	info("Version: %d.%d.\n", CONFIG_MAJOR_VERSION, CONFIG_MINOR_VERSION);
	cfg.bver = CONFIG_MAJOR_VERSION;
	entry++;
	cfg.cver = CONFIG_MINOR_VERSION;
	entry++;
	cfg.fs_addr = arg_parse_value(argv[arg++], entry);
		
	printf("Writing configuration image to file %s.\n", image_filename);
	errno = 0;
	fp = fopen(image_filename, "w");
	if (!fp || (errno > 0))
	{
		die("Could not open image file.");
	}
	//Write 1st and 2nd sector.
	for (sector = 0; sector < 2; sector++)
	{
		/* All configuration write is unrolled because my logic is flawed,
		 * and tells me that it'll force me to keep track of updating 
		 * configuration entries.
		 */
		//Write configuration signature.
		entry = 0;
		errno = 0;
		if ((fwrite(&cfg.signature, sizeof(uint8_t), sizeof(cfg.signature), fp) != sizeof(cfg.signature)) || (errno > 0))
		{
			die("Could not write configuration signature.");
		}
		config_size = sizeof(cfg.signature);
		//Write major version.
		entry++;
		errno = 0;
		if ((fwrite(&cfg.bver, sizeof(uint8_t), sizeof(cfg.bver), fp) != sizeof(cfg.bver)) || (errno > 0))
		{
			die("Could not write configuration version.");
		}
		config_size += sizeof(cfg.bver);
		//Write minor version.
		entry++;
		errno = 0;
		if ((fwrite(&cfg.cver, sizeof(uint8_t), sizeof(cfg.cver), fp) != sizeof(cfg.cver)) || (errno > 0))
		{
			die("Could not write configuration version.");
		}
		config_size += sizeof(cfg.cver);
		//Write file system address.
		entry++;
		errno = 0;
		if ((fwrite(&cfg.fs_addr, sizeof(uint8_t), sizeof(cfg.fs_addr), fp) != sizeof(cfg.fs_addr)) || (errno > 0))
		{
			die("Could not write file system address.");
		}
		config_size += sizeof(cfg.fs_addr);
		
		//Write the rest of the sector.
		for (i = config_size; i < 4096; i++)
		{
			errno = 0;
			fputc(0, fp);
			if (errno >0)
			{
				die("Error writing padding.\n");
			}
		}
	}
	errno = 0;
	fclose(fp);
	if (errno > 0)
	{
		die("Error closing image file.");
	}
	printf("Configuration image written.\n");
	return(EXIT_SUCCESS);
}
