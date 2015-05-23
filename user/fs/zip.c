/**
 * @file zip.c
 * @brief Basic routines for parsing an uncompressed ZIP file.
 *
 * A ZIP file is used to represent a file system, on the ESP8266.
 * The suggestion, to use a ZIP file this way, was made by [Dave Hylands](http://www.davehylands.com/), 
 * and the code is based on his original work.
 *  
 * ZIP file structure (from [.ZIP Application Note](https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT):
 * 
 *     [local file header 1]
 *     [encryption header 1]
 *     [file data 1]
 *     [data descriptor 1]
 *     . 
 *     .
 *     .
 *     [local file header n]
 *     [encryption header n]
 *     [file data n]
 *     [data descriptor n]
 *     [archive decryption header] 
 *     [archive extra data record] 
 *     [central directory header 1]
 *     .
 *     .
 *     .
 *     [central directory header n]
 *     [zip64 end of central directory record]
 *     [zip64 end of central directory locator] 
 *     [end of central directory record]
 * 

 * In this implementation, only the bare minimum data are read from the ZIP 
 * file. The code assumes that the ZIP file starts with a "local file header".
 * This is true in most cases, though reading the specs, it seems to me this
 * might not alway be the case.
 * From the "local file header" only useful information is parsed, and used to
 * locate the file data. Since the ZIP structure is only used to emulate a file
 * system structure, no decompression is implemented.
 *
 * Based on: https://github.com/micropython/micropython/blob/master/teensy/memzip.c
 */

#include "osapi.h"
#include "tools/missing_dec.h"
#include "int_flash.h"
#include "user_config.h"
#include "zip.h"

/**
 * @brief Load a local file header.
 * 
 * @param address Address of the header to load.
 * @return A pointer to the header data.
 */
static struct zip_file_hdr ICACHE_FLASH_ATTR *zip_load_header(unsigned int address)
{
    struct zip_file_hdr *file_hdr = db_malloc(sizeof(struct zip_file_hdr));
    uint32_t signature;
    unsigned int offset = address;
    
    debug("Loading header at 0x%x.\n", offset);
    
    //Test the signature
    if (!flash_read(&signature, offset, sizeof(signature)))
    {
        debug("No more files, reading the ZIP file header signature at %d.\n", address);
        db_free(file_hdr);
        return(NULL);
    }
    debug(" Header signature: 0x%x.\n", signature);
    
    //Skip over signature and "version needed extract".
    offset += 6;
    //Read the flags, compression method, last modified time and date.
    if (!flash_read(&file_hdr->flags, offset, sizeof(uint16_t) * 4))
    {
        error("Failed reading the ZIP flags at %d.\n", offset);
        db_free(file_hdr);
        return(NULL);
    }
    //Skip crc32, and compressed size
    offset += (sizeof(uint16_t) * 4) + 8;
    //Read uncompressed data size, file name length, and extra data length.
    if (!flash_read(&file_hdr->uncompressed_size, offset, sizeof(uint32_t) * 2))
    {
        error("Failed reading the ZIP file size at %d.\n", offset);
        db_free(file_hdr);
        return(NULL);
    }
    //Alloc an extra byte for the \0 byte.
    file_hdr->filename = db_malloc(file_hdr->filename_len + 1);
    //Load the file name.
    if (!flash_read(file_hdr->filename, 
                    address + ZIP_REAL_FILE_HEADER_SIZE, 
                    file_hdr->filename_len))
    {
        error("Failed loading the ZIP file name at %d.\n",
              address + ZIP_REAL_FILE_HEADER_SIZE);
        zip_free_header(file_hdr);
        return(NULL);
    }
    //Terminate the string.
    file_hdr->filename[file_hdr->filename_len] = '\0';
    debug(" File name: %s.\n", file_hdr->filename);
    
    return(file_hdr);
}

/**
 * @brief Find a entry in the ZIP file.
 * 
 * @param path Name of the entry to find.
 * @return A pointer to the ZIP header for the file.
 */
struct zip_file_hdr *zip_find_file_header(const char *path) 
{
    struct zip_file_hdr *file_hdr;
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
    //Run while there are more headers.
    while (file_hdr) 
    {
        //Test bit 3 of the flags, to see if a data descriptor is used.
        if ((file_hdr->flags & ( 1 << 2)))
        {
            error("ZIP data descriptors are not supported.\n");
            return(NULL);
        }
        //Calculate the position of the file data.
        offset += ZIP_REAL_FILE_HEADER_SIZE;
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
        zip_free_header(file_hdr);
        
        //Skip to next header.
        offset += file_hdr->uncompressed_size;
        //Read it.
        file_hdr = zip_load_header(offset);
        if (!file_hdr)
        {
            return(NULL);
        }    
    }
    zip_free_header(file_hdr);
    debug("Not found!\n"); 
    return NULL;
}

/**
 * @brief Check if the path is a directory.
 * 
 * @param path Path to check.
 * @return True if path is a directory.
 */
bool zip_is_dir(const char *path) 
{
    const struct zip_file_hdr *file_hdr;
    size_t filename_len;

    if (os_strcmp(path, "/") == 0) 
    {
        // The root directory is a directory.
        return(true);
    }

    file_hdr = zip_find_file_header(path);
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

/**
 * @brief Free up memory used by a file header.
 * 
 * @param file_hdr Pointer to the memory to free.
 */
void ICACHE_FLASH_ATTR zip_free_header(struct zip_file_hdr *file_hdr)
{
    db_free(file_hdr->filename);
    db_free(file_hdr);
}
