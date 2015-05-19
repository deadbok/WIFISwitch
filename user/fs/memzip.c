/**
 * @file memzip.c
 * @brief Basic routines for parsing an uncompressed ZIP file.
 */
//Ripped from micropython: https://github.com/micropython
//Credits: Dave Hylands

//#include <stdint.h>
//#include <stdlib.h>
//#include <string.h>
#include "osapi.h"
#include "int_flash.h"
#include "user_config.h"
#include "memzip.h"

/**
 * @brief Find a file in the ZIP file.
 * 
 * @param filename Name of the file to find.
 * @return A pointer to the ZIP header for the file.
 */
const struct int_file_hdr *memzip_find_file_header(const char *filename) 
{

    struct int_file_hdr *file_hdr;
    unsigned int offset = 0;

    debug("Finding file header for file: %s.\n", filename);
    file_hdr = zip_load_header(0);

    /* Zip file filenames don't have a leading /, so we strip it off */

    if (*filename == '/') 
    {
        filename++;
    }
    while (file_hdr->signature == MEMZIP_FILE_HEADER_SIGNATURE) 
    {
        if (!os_strncmp(file_hdr->filename, filename, file_hdr->filename_len)) 
        {
            /* We found a match */
            debug("Found.\n");
            return(file_hdr);
        }
        offset += sizeof(MEMZIP_FILE_HDR);
        offset += file_hdr->filename_len;
        offset += file_hdr->extra_len;
        offset += file_hdr->uncompressed_size;
        
        zip_free_header(file_hdr);
        
        file_hdr = zip_load_header(offset);
    }
    zip_free_header(file_hdr);
    debug("Not found!\n"); 
    return NULL;
}

bool memzip_is_dir(const char *filename) 
{
    const struct int_file_hdr *file_hdr;
    size_t filename_len;

    if (os_strcmp(filename, "/") == 0) 
    {
        // The root directory is a directory.
        return(true);
    }

    file_hdr = memzip_find_file_header(filename);
    if (file_hdr == NULL)
    {
        return(false);
    }
    // Zip filenames don't have a leading /, so we strip it off
    if (*filename == '/') 
    {
        filename++;
    }
    filename_len = strlen(filename);

    if (filename_len < file_hdr->filename_len &&
        os_strncmp(file_hdr->filename, filename, filename_len) == 0 &&
        file_hdr->filename[filename_len] == '/') 
    {
        return(true);
    }
    return(false);
}

MEMZIP_RESULT memzip_locate(const char *filename, void **data, size_t *len)
{
    const struct int_file_hdr *file_hdr = memzip_find_file_header(filename);
    if (file_hdr == NULL) 
    {
        return MZ_NO_FILE;
    }
    if (file_hdr->compression_method != 0) 
    {
        return MZ_FILE_COMPRESSED;
    }

    uint8_t *mem_data;
    mem_data = (uint8_t *)&file_hdr[1];
    mem_data += file_hdr->filename_len;
    mem_data += file_hdr->extra_len;

    *data = mem_data;
    *len = file_hdr->uncompressed_size;
    return MZ_OK;
}

MEMZIP_RESULT memzip_stat(const char *path, MEMZIP_FILE_INFO *info) {
    const struct int_file_hdr *file_hdr = memzip_find_file_header(path);
    if (file_hdr == NULL) 
    {
        if (memzip_is_dir(path)) 
        {
            info->file_size = 0;
            info->last_mod_date = 0;
            info->last_mod_time = 0;
            info->is_dir = 1;
            return MZ_OK;
        }
        return MZ_NO_FILE;
    }
    info->file_size = file_hdr->uncompressed_size;
    info->last_mod_date = file_hdr->last_mod_date;
    info->last_mod_time = file_hdr->last_mod_time;
    info->is_dir = 0;

    return MZ_OK;
}
