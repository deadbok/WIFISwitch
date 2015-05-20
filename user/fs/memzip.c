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
#include "tools/missing_dec.h"
#include "int_flash.h"
#include "user_config.h"
#include "memzip.h"

/**
 * @brief Find a entry in the ZIP file.
 * 
 * @param path Name of the entry to find.
 * @return A pointer to the ZIP header for the file.
 */
struct int_file_hdr *memzip_find_file_header(const char *path) 
{
    struct int_file_hdr *file_hdr;
    unsigned int offset = 0;

    debug("Finding file header for path: %s.\n", path);
    file_hdr = zip_load_header(0);
    if (!file_hdr)
    {
        return(NULL);
    }    

    //Zip file filenames don't have a leading /, so we strip it off.
    if (*path == '/') 
    {
        path++;
    }
    //Run while the signature is right.
    while (file_hdr->signature == MEMZIP_FILE_HEADER_SIGNATURE) 
    {
        //Calculate the position of the file data.
        offset += sizeof(MEMZIP_FILE_HDR);
        offset += file_hdr->filename_len;
        offset += file_hdr->extra_len;
        
        if (!os_strncmp(file_hdr->filename, path, file_hdr->filename_len)) 
        {
            //We found a match .
            debug("Found.\n");
            file_hdr->data_pos = offset;
            return(file_hdr);
        }
        
        //Free the memory again
        memzip_free_header(file_hdr);
        
        //Skip to next header.
        offset += file_hdr->uncompressed_size;
        //Read it.
        file_hdr = zip_load_header(offset);
        if (!file_hdr)
        {
            return(NULL);
        }    
    }
    memzip_free_header(file_hdr);
    debug("Not found!\n"); 
    return NULL;
}

/**
 * @brief Check if the path is a directory.
 * 
 * @param path Path to check.
 * @return True if path is a directory.
 */
bool memzip_is_dir(const char *path) 
{
    const struct int_file_hdr *file_hdr;
    size_t filename_len;

    if (os_strcmp(path, "/") == 0) 
    {
        // The root directory is a directory.
        return(true);
    }

    file_hdr = memzip_find_file_header(path);
    if (!file_hdr)
    {
        return(false);
    }
    // Zip filenames don't have a leading /, so we strip it off
    if (*path == '/') 
    {
        path++;
    }
    filename_len = strlen(path);

    if (filename_len < file_hdr->filename_len &&
        os_strncmp(file_hdr->filename, path, filename_len) == 0 &&
        file_hdr->filename[filename_len] == '/') 
    {
        return(true);
    }
    return(false);
}

MEMZIP_RESULT memzip_stat(const char *path, MEMZIP_FILE_INFO *info) {
    const struct int_file_hdr *file_hdr = memzip_find_file_header(path);
    if (!file_hdr) 
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

void ICACHE_FLASH_ATTR memzip_free_header(struct int_file_hdr *file_hdr)
{
    db_free(file_hdr->filename);
    db_free(file_hdr);
}
