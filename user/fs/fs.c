/** 
 * @file fs.h
 *
 * @brief Routines for file access.
 *
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@ace2>
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
#include "c_types.h"
#include "user_config.h"
#include "int_flash.h"
#include "memzip.h"
#include "fs.h"

/**
* @brief Structure to keep file information on open files.
*/
struct fs_file
{
    /**
     * @brief Start position of the file data.
     */
    unsigned int start_pos;
    /**
     * @brief Current position in the file data.
     */
    unsigned int pos;
    /**
     * @brief Size of the file data.
     */
    size_t size;
};

/**
 * @brief Array of all open files.
 */
static struct fs_file *fs_open_files[FS_MAX_OPEN_FILES];
static FS_FILE_H n_open_files = 0;

/**
 * @brief Open a file.
 * 
 * @param filename Name of the file to open.
 * @return A handle to the newly opened file, or -1 on error.
 */
FS_FILE_H ICACHE_FLASH_ATTR fs_open(const char *filename)
{
    struct int_file_hdr *file_hdr;
    struct fs_file *file;
    FS_FILE_H i;
    
    debug("Opening file: %s.\n", filename);
    if (n_open_files == FS_MAX_OPEN_FILES)
    {
        os_printf("ERROR: Maximum number of open files reached.\n");
        return(-1);
    }
    
    file_hdr = memzip_find_file_header(filename);
    if (!file_hdr)
    {
        os_printf("ERROR: Could not open %s.\n", filename);
        return(-1);
    }
    
    file = db_malloc(sizeof(struct fs_file));
    file->pos = 0;
    file->start_pos = file_hdr->data_pos;
    file->size = file_hdr->uncompressed_size;
    
    memzip_free_header(file_hdr);
    
    for(i = 0; ((i < FS_MAX_OPEN_FILES) && (fs_open_files[i] != NULL)); i++);
    if (i == FS_MAX_OPEN_FILES)
    {
        os_printf("ERROR: Could not add file to array of open files.\n");
        return(-1);
    }
    fs_open_files[i] = file;    
    debug(" File handle: %d.\n", i);
    return(i);
}

/**
 * @brief Close an open file.
 * 
 * @param handle Handle of the file to close.
 */
void ICACHE_FLASH_ATTR fs_close(FS_FILE_H handle)
{
    debug("Closing file handle %d.\n", handle);
    
    if (handle >= FS_MAX_OPEN_FILES)
    {
        os_printf("ERROR: Impossible file handle given %d.\n", handle);
        return;
    }
    db_free(fs_open_files[handle]);
    fs_open_files[handle] = NULL;
}

/**
 * @brief Read an amount of stuff from a file.
 * 
 * @param buffer Pointer to the array where the read objects are stored.
 * @param size 	Size of each object in bytes.
 * @param count The number of the objects to be read.
 * @param handle The handle of the file to read from.
 */
size_t ICACHE_FLASH_ATTR fs_read(void *buffer, size_t size, size_t count, FS_FILE_H handle)
{
    size_t total_size = size * count;
    
    debug("Reading %d bytes from %d.\n", total_size, handle);
    if (!flash_read(buffer, fs_open_files[handle]->start_pos, total_size))
    {
        os_printf("ERROR: Failed reading %d bytes from %d.\n", total_size, handle);
        return(0);
    }
    return(count);
}
