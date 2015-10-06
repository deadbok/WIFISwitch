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
/**
 * @brief Use X/Open 5, incorporating POSIX 1995 (for nftw).
 */
#define _XOPEN_SOURCE 500 //For nftw.

#include <errno.h> //errno
#include <stdio.h> //printf
#include <stddef.h> //size_t
#include <string.h> //strlen
#include <stdint.h> //Fixed width integer types.
#include <stdlib.h> //malloc, abort
#include "common.h"
#include "dbffs.h"
#include "dbffs-gen.h"
#include "heatshrink_encoder.h"

/** 
 * @brief Compress the file data using Heatshrink.
 * 
 * @param data Pointer to file data.
 * @param data_size Size of file data.
 * @param buf Pointer to output buffer.
 * @param buf_size Buffer size.
 * @return Number of compressed bytes in buf.
 */
static size_t encode_file_data(uint8_t *data, size_t data_size, uint8_t *buf, size_t buf_size)
{
	heatshrink_encoder *henc = NULL;
	HSE_sink_res sink_res;
	HSE_poll_res poll_res;
	HSE_finish_res finish_res;
	uint8_t *buf_pos = buf;
	size_t sunk;
	size_t polled;
	size_t ret = 0;
	
	henc = heatshrink_encoder_alloc(10, 5);
	if (!henc)
	{
		die("Could not allocate Heatshrink encoder.");
	}
		
	while (data_size > 0)
	{
		sink_res = heatshrink_encoder_sink(henc, data, data_size, &sunk);
		if (sink_res != HSER_SINK_OK)
		{
			die("Error buffering file data.");
		}
		data_size -= sunk;
		data += sunk;
		do
		{
			if (data_size == 0)
			{
				finish_res = heatshrink_encoder_finish(henc);
				if (finish_res == HSER_FINISH_DONE)
				{
					break;
				}
				if (finish_res != HSER_FINISH_MORE)
				{
					die("Error closing encoder.");
				}
			}
			poll_res = heatshrink_encoder_poll(henc, buf_pos, buf_size, &polled);
			if ((poll_res != HSER_POLL_MORE) &&
			    (poll_res != HSER_POLL_EMPTY))
			{
				die("Error encoding file data.");
			}
			buf_pos += polled;
			buf_size -= polled;
			ret += polled;
		} while (poll_res == HSER_POLL_MORE);
	}
    return(ret);
}

struct dbffs_file_hdr *create_file_entry(const char *path, const char *entryname)
{
	FILE *fp;
	size_t bytes_read;
	struct dbffs_file_hdr *entry;	
	
	info("  Creating file %s source ", entryname);
	//Get mem and start populating a file entry.
	errno = 0;
	entry = calloc(sizeof(struct dbffs_file_hdr), sizeof(uint8_t));
	if (!entry || (errno > 0))
	{
		die("Could not allocate memory for file entry.");
	}
	entry->signature = DBFFS_FILE_SIG;
	if (!(entry->name = calloc(strlen(entryname) + 1, sizeof(char))))
	{
		die("Could not allocate memory for file entry name.");
	}
	strcpy(entry->name, entryname);
	entry->name_len  = strlen(entryname);
	//Find file data length.
	info("%s.\n", path);
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
	info("  Data size %d.\n", entry->size);
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
	if (!(entry->cdata = malloc(sizeof(uint8_t) * entry->size * 2)))
	{
		die("Could not allocate memory for compressed file data.");
	}
	entry->csize = encode_file_data(entry->data, entry->size, entry->cdata, entry->size * 2);
	info("  Compressed size %d.\n", entry->csize);
	return(entry);
}

uint32_t write_file_entry(const struct dbffs_file_hdr *entry, FILE *fp)
{
	size_t ret;
	uint32_t offset;
	bool compressed;
	uint32_t csize;
	
	//Write signature.
	errno = 0;
	ret = fwrite(&entry->signature, sizeof(uint8_t), sizeof(entry->signature), fp);
	if ((ret != sizeof(entry->signature)) || (errno > 0))
	{
		die("Could not write file entry signature.");
	}
	if (entry->next)
	{
		if (entry->csize < entry->size)
		{
			//Save compressed data.
			compressed = true;
			csize = entry->csize;
			offset = sizeof(entry->signature) + 4 + //signature(4) + offset(4) +
					 sizeof(entry->name_len) + //name length(1) +
					 entry->name_len + sizeof(entry->size) + //name_len + data_size(4) + 
					 sizeof(entry->csize) + entry->csize; //compressed_data_size(4) + compressed_data_size

		}
		else
		{
			//Save uncompressed data.
			compressed = false;
			csize = 0;
			//Calculate offset of the next entry.
			offset = sizeof(entry->signature) + 4 + //signature(4) + offset(4) +
					 sizeof(entry->name_len) + //name length(1) +
					 entry->name_len + sizeof(entry->size) + //name_len + data_size(4) + 
					 sizeof(entry->csize) + entry->size; //compressed_data_size(4) + data_size
		}
	}
	else
	{
		offset = 0;
	}

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
	//Write compressed data size.
	errno = 0;
	ret = fwrite(&csize, sizeof(uint8_t), sizeof(csize), fp);
	if ((ret != sizeof(csize)) || (errno > 0))
	{
		die("Could not write file data size.");
	}
	errno = 0;	
	if (compressed)
	{
		//Write compressed data.
		ret = fwrite(entry->cdata, sizeof(uint8_t), entry->csize, fp);
		if ((ret != entry->csize) || (errno > 0))
		{
			die("Could not write compressed file data.");
		}
	}
	else
	{
		//Write data.
		ret = fwrite(entry->data, sizeof(uint8_t), entry->size, fp);
		if ((ret != entry->size) || (errno > 0))
		{
			die("Could not write file data.");
		}
	}
	return(offset);
}
