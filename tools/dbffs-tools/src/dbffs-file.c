/** 
 * @file dbffs-file.c
 *
 * @brief File related DBFFS.
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
#include <stddef.h> //size_t
#include <string.h> //strlen
#include <stdint.h> //Fixed width integer types.
#include <stdlib.h> //malloc, abort
#include "common.h"
#include "dbffs.h"
#include "dbffs-gen.h"

struct dbffs_file_hdr *create_file_entry(const char *path, char *entryname)
{
	FILE *fp;
	size_t bytes_read;
	struct dbffs_file_hdr *entry;	
	
	printf("file\n");
	//Get mem and start populating a file entry.
	errno = 0;
	entry = calloc(sizeof(struct dbffs_file_hdr), sizeof(uint8_t));
	if (!entry || (errno > 0))
	{
		die("Could not allocate memory for file entry.");
	}
	entry->signature = DBFFS_FILE_SIG;
	entry->name = entryname;
	entry->name_len  = strlen(entryname);
	//Find file data length.
	errno = 0;
	fp = fopen(path, "r");
	if (!fp || (errno > 0))
	{
		die("Could not open file.");
	}
	errno = 0;
	fseek(fp, 0, SEEK_END);
	if (errno > 0)
	{
		die("Could not change position to the end of file.");
	}
	errno = 0;
	entry->size = ftell(fp);
	if (errno > 0)
	{
		die("Could not get position in file.");
	}
	//Go back to start, allocate memory for the data and read them into memory.
	errno = 0;
	fseek(fp, 0, SEEK_SET);
	if (errno > 0)
	{
		die("Could not change position to the start of file.");
	}
	errno = 0;
	entry->data = malloc(sizeof(uint8_t) * entry->size);
	if (!entry || (errno > 0))
	{
		die("Could not allocate memory for the file data.");
	}			
	errno = 0;
	bytes_read = fread(entry->data,
		  sizeof(uint8_t),
		  entry->size, 
		  fp);
	if ((bytes_read < entry->size) || (errno > 0))
	{
		die("Error reading data from file.\n");
	}
	//Close file.
	errno = 0;
	fclose(fp);
	if (errno > 0)
	{
		die("Error closing file.");
	}
	return(entry);
}

void write_file_entry(const struct dbffs_file_hdr *entry, FILE *fp)
{
	size_t ret;
	uint32_t offset;
	uint32_t dword;
	
	//Write signature.
	errno = 0;
	ret = fwrite(&entry->signature, sizeof(uint8_t), sizeof(entry->signature), fp);
	if ((ret != sizeof(entry->signature)) || (errno > 0))
	{
		die("Could not write file entry signature.");
	}
	//Calculate offset of the next entry.
	offset = swap32(sizeof(entry->signature) + 4 + //signature(4) + offset(4) +
				    sizeof(entry->name_len) + //name length(1) +
				    entry->name_len + sizeof(entry->size) + //name_len + data_size(4) + 
				    entry->size); //data_size
	//Write offset to next entry.
	errno = 0;
	ret = fwrite(&offset, sizeof(uint8_t), sizeof(offset), fp);
	if ((ret != sizeof(offset)) || (errno > 0))
	{
		die("Could not write offset to next entry.");
	}
	//Write name length.
	errno = 0;
	ret = fwrite(&entry->name_len, sizeof(uint8_t), sizeof(entry->name_len), fp);
	if ((ret != sizeof(entry->name_len)) || (errno > 0))
	{
		die("Could not write file name length.");
	}
	//Write name.
	errno = 0;
	ret = fwrite(entry->name, sizeof(uint8_t), entry->name_len, fp);
	if ((ret != entry->name_len) || (errno > 0))
	{
		die("Could not write file name.");
	}
	//Write data size.
	errno = 0;
	ret = fwrite(&entry->size, sizeof(uint8_t), sizeof(entry->size), fp);
	if ((ret != sizeof(entry->size)) || (errno > 0))
	{
		die("Could not write file data size.");
	}
	//Write data.
	errno = 0;
	ret = fwrite(entry->data, sizeof(uint8_t), entry->size, fp);
	if ((ret != entry->size) || (errno > 0))
	{
		die("Could not write file data.");
	}
}
