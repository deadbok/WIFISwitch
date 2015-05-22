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
#include "zip.h"
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
    /**
     * @brief Set on end of file.
     */
     bool eof;
};

/**
 * @brief Array of all open files.
 */
static struct fs_file *fs_open_files[FS_MAX_OPEN_FILES];
static FS_FILE_H n_open_files = 0;

/**
 * @brief Initialise stuff for file system access.
 */
void ICACHE_FLASH_ATTR fs_init(void)
{
}

static bool ICACHE_FLASH_ATTR fs_test_handle(FS_FILE_H handle)
{
    if ((handle > -1) && (handle < FS_MAX_OPEN_FILES))
    {
        debug("Valid file handle.\n");
        return(true);
    }
    error("Invalid file handle.\n");
    return(false);
}

/**
 * @brief Open a file.
 * 
 * @param filename Name of the file to open.
 * @return A handle to the newly opened file, or -1 on error.
 */
FS_FILE_H ICACHE_FLASH_ATTR fs_open(const char *filename)
{
    struct zip_file_hdr *file_hdr;
    struct fs_file *file;
    FS_FILE_H i;
    
    debug("Opening file: %s.\n", filename);
    if (n_open_files == FS_MAX_OPEN_FILES)
    {
        error("Maximum number of open files reached.\n");
        return(-1);
    }
    
    file_hdr = zip_find_file_header(filename);
    if (!file_hdr)
    {
        error("Could not open %s.\n", filename);
        return(-1);
    }
    
    file = db_malloc(sizeof(struct fs_file));
    file->pos = 0;
    file->start_pos = file_hdr->data_pos;
    file->size = file_hdr->uncompressed_size;
    file->eof = false;
    
    zip_free_header(file_hdr);
    
    for(i = 0; ((i < FS_MAX_OPEN_FILES) && (fs_open_files[i] != NULL)); i++);
    if (i == FS_MAX_OPEN_FILES)
    {
        error("Could not add file to array of open files.\n");
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
    
    if (!fs_test_handle(handle))
    {
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
 * @return The count of objects read.
 */
size_t ICACHE_FLASH_ATTR fs_read(void *buffer, size_t size, size_t count, FS_FILE_H handle)
{
    size_t total_size = size * count;
    
    debug("Reading %d bytes from %d.\n", total_size, handle);
    if (!fs_test_handle(handle))
    {
        return(0);
    }
    if( total_size > fs_open_files[handle]->size)
    {
        warn("Truncating read to file size.\n");
        total_size = fs_open_files[handle]->size;
    }
    if ( total_size == fs_open_files[handle]->size)
    {
        fs_open_files[handle]->eof = true;
    }

    if (!flash_read(buffer, fs_open_files[handle]->start_pos, total_size))
    {
        error("Failed reading %d bytes from %d.\n", total_size, handle);
        return(0);
    }
    fs_open_files[handle]->pos = total_size;
    return(total_size / count);
}

int ICACHE_FLASH_ATTR fs_getc(FS_FILE_H handle)
{
    int ch;
    unsigned int abs_pos = fs_open_files[handle]->start_pos + fs_open_files[handle]->pos;

    debug("Reading a character from %d.\n", handle);
    if (!fs_test_handle(handle))
    {
        return(FS_EOF);
    }
    if (fs_open_files[handle]->pos >= fs_open_files[handle]->size)
    {
        return(FS_EOF);
    }
    
    if (!flash_read(&ch, abs_pos, sizeof(char)))
    {
        error("Failed reading %d bytes from %d.\n", sizeof(char), handle);
        return(FS_EOF);
    }
    fs_open_files[handle]->pos += sizeof(char);
    return(ch);
}    
