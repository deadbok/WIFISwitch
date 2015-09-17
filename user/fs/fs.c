/** 
 * @file fs.c
 *
 * @brief Routines for file access.
 * 
 * These functions mimics some of the file functions in the standard C library.
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
#include "c_types.h"
#include "user_config.h"
#include "int_flash.h"
#include "dbffs.h"
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
/**
 * @brief Number currently open files.
 */
static FS_FILE_H n_open_files = 0;

/**
 * @brief Initialise stuff for file system access.
 */
void ICACHE_FLASH_ATTR fs_init(void)
{
	db_printf("ROM size %d KiB.\n", flash_size() >> 10);
	init_dbffs();
}

/**
 * @brief Test if a file handle is valid.
 * 
 * @param handle The handle to test.
 * @return True if this is a valid handle, false otherwise.
 */
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
 * @brief Check if the end of the file has been reached.
 * 
 * Check for EOF, and set file handle accordingly.
 * 
 * @param handle The handle of the file to check.
 * @return True on end of file, false otherwise.
 */
static bool ICACHE_FLASH_ATTR fs_check_eof(FS_FILE_H handle)
{
    //End of file?
    if (fs_open_files[handle]->pos >= fs_open_files[handle]->size)
    {
        //Read doesn't happen.
        fs_open_files[handle]->pos = fs_open_files[handle]->size;
        fs_open_files[handle]->eof = true;
        debug("End of file: %d of %d.\n", fs_open_files[handle]->pos,
              fs_open_files[handle]->size);
    }
    return(fs_open_files[handle]->eof);
}

/**
 * @brief Open a file.
 * 
 * @param filename Name of the file to open.
 * @return A handle to the newly opened file, or -1 on error.
 */
FS_FILE_H ICACHE_FLASH_ATTR fs_open(char *filename)
{
    struct dbffs_file_hdr *file_hdr;
    struct fs_file *file;
    FS_FILE_H i;
    
    debug("Opening file: %s.\n", filename);
    if (n_open_files == FS_MAX_OPEN_FILES)
    {
        error("Maximum number of open files reached.\n");
        return(-1);
    }
    
    file_hdr = dbffs_find_file_header(filename);
    if (!file_hdr)
    {
        debug("Could not open %s.\n", filename);
        return(-1);
    }
   
    //Get some memory for file house keeping, and fill in the data.
    file = db_malloc(sizeof(struct fs_file), "file fs_open");
    file->pos = 0;
    file->start_pos = file_hdr->data_addr;
    file->size = file_hdr->size;
    file->eof = false;
    
    //Free up the ZIP header, since we don't need it anymore.
    //zip_free_header(file_hdr);
    
    //Find the first free file handle
    for(i = 0; ((i < FS_MAX_OPEN_FILES) && (fs_open_files[i] != NULL)); i++);
    if (i == FS_MAX_OPEN_FILES)
    {
        error("Could not add file to array of open files.\n");
        return(-1);
    }
    //Assign to current file
    fs_open_files[i] = file;    
    debug(" File handle: %d.\n", i);
    debug(" Size: %d.\n", file->size);
    debug(" Start: 0x%x.\n", file->start_pos);
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
    if (fs_open_files[handle])
    {
		db_free(fs_open_files[handle]);
	}
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
    unsigned int abs_pos = fs_open_files[handle]->start_pos + fs_open_files[handle]->pos;
    
    debug("Reading %d bytes from %d.\n", total_size, handle);
    if (!fs_test_handle(handle))
    {
        return(0);
    }
    //Don't read beyond the data.
    if (total_size > (fs_open_files[handle]->size - fs_open_files[handle]->pos))
    {
        warn("Truncating read to file size.\n");
        total_size = fs_open_files[handle]->size - fs_open_files[handle]->pos;
    }

    if (!aflash_read(buffer, abs_pos, total_size))
    {
        error("Failed reading %d bytes from %d.\n", total_size, handle);
        return(0);
    }
    fs_open_files[handle]->pos += total_size;
    fs_check_eof(handle);
    return(count);
}

/**
 * @brief Read a character from a file.
 * 
 * @param handle A handle to the file to read from.
 * @return The character read, or #FS_EOF on failure.
 */ 
int ICACHE_FLASH_ATTR fs_getc(FS_FILE_H handle)
{
    int ch;
    unsigned int abs_pos = fs_open_files[handle]->start_pos + fs_open_files[handle]->pos;

    debug("Reading a character from %d.\n", handle);
    if (!fs_test_handle(handle) || fs_check_eof(handle))
    {
        return(FS_EOF);
    }
    
    //Read the char.
    if (!aflash_read(&ch, abs_pos, sizeof(char)))
    {
        error("Failed reading %d bytes from %d.\n", sizeof(char), handle);
        return(FS_EOF);
    }
    //Adjust position.
    fs_open_files[handle]->pos += sizeof(char);
    return(ch);
}    

/**
 * @brief Read a string from a file.
 * 
 * Read a string from a file.  Read `count - 1` characters from the file and put
 * them in str, ending with a `'\0'`. Stop on a `'\0'`, or an end of file. If a
 * new line is read, it is included and ends the string (besides the `'\0'`.
 * 
 * @param str Pointer to the string to read the characters to.
 * @param count Maximum number of characters to read.
 * @param handle A handle to the file to read from.
 * @return The string read, or NULL on error.
 */ 
char ICACHE_FLASH_ATTR *fs_gets(char *str, size_t count, FS_FILE_H handle)
{
    unsigned int abs_pos = fs_open_files[handle]->start_pos + fs_open_files[handle]->pos;
    unsigned int i = 0;
    unsigned char ch;
    
    debug("Reading a string of max. %d characters from %d.\n", count, handle);
    if (!fs_test_handle(handle) || fs_check_eof(handle))
    {
        return(NULL);
    }
    
    //Read the string, Stop on end of file, new lines, and zero bytes.
    do
    {
        //Read the char.
        if (!aflash_read(&ch, abs_pos + i, sizeof(char)))
        {
            error("Failed reading %d bytes from %d.\n", sizeof(char), handle);
            return(NULL);
        }
        //Update position.
        fs_open_files[handle]->pos += sizeof(char);
        //Add char.
        str[i++] = ch;
    }
    while ((i < (count - 1)) && (!fs_check_eof(handle)) && 
                 (ch != '\0') && (ch != '\n'));
    if (ch)
    {
        //Zero terminate.
        str[i] = '\0';
    }
    
    return(str);
}

/**
 * @brief Get the current position relative to the start of the file.
 * 
 * @param handle Handle to a file.
 * @return Position or FS_EOF 
 */
long ICACHE_FLASH_ATTR fs_tell( FS_FILE_H handle)
{
    if (!fs_test_handle(handle))
    {
        return(FS_EOF);
    }
    
    return(fs_open_files[handle]->pos);
}

/**
 * @brief Get the file size.
 * 
 * @param handle Handle to a file.
 * @return Size of file or FS_EOF 
 */
long ICACHE_FLASH_ATTR fs_size( FS_FILE_H handle)
{
    if (!fs_test_handle(handle))
    {
        return(FS_EOF);
    }
    
    return(fs_open_files[handle]->size);
}

/**
 * @brief Set the current position in the file.
 * 
 * Origin values:
 *  - SEEK_CUR: Set position by adding offset to the current position.
 *  - SEEK_END: Set position by substracting offset to the end position.
 *  - SEEK_SET: Set position by adding offset to the start position.* 
 * 
 * @param handle Handle to a file.
 * @param offset New position in file.
 * @param origin FS_SEEK_CUR, FS_SEEK_SET, or FS_SEEK_END, from where to set the
 *               offset.
 * @return Position or FS_EOF 
 */
int ICACHE_FLASH_ATTR fs_seek(FS_FILE_H handle, long offset, fs_seek_pos_t origin)
{
    if (!fs_test_handle(handle))
    {
        return(FS_EOF);
    }
    
    switch(origin)
    {
        case FS_SEEK_CUR: fs_open_files[handle]->pos += offset;
                          break;
                          
        case FS_SEEK_END: fs_open_files[handle]->pos = fs_open_files[handle]->size - offset;
                          break;
        case FS_SEEK_SET: fs_open_files[handle]-> pos = offset;
                          break;
        default: warn("Unknown file origin requested.\n");
    }
    fs_check_eof(handle);
    
    return(0);
}

/**
 * @brief Get end of file status.
 * 
 * @param handle Handle to a file.
 * @return 0 if the end is not reached or FS_EOF
 */
int ICACHE_FLASH_ATTR fs_eof(FS_FILE_H handle)
{
    if (!fs_test_handle(handle))
    {
        return(FS_EOF);
    }

    return(fs_check_eof(handle));
}
