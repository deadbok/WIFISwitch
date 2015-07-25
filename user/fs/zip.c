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
 * @brief Entry in the file look up table.
 */
struct zip_flut_entry
{
	/**
	 * @brief File name.
	 */
	char *filename;
	/**
	 * @brief Address of the file header in the ZIP file.
	 */
	unsigned int hdr_offset;
	/**
	 * @brief Address of the file data in the ZIP file.
	 */
	unsigned int data_offset;
};
/**
 * @brief File look up table.
 */
static struct zip_flut_entry *zip_flut;
/**
 * @brief Number of entries in the flut.
 */
static unsigned short zip_flut_entries;

/**
 * @brief Load a local file header.
 * 
 * @param address Address of the header to load.
 * @return A pointer to the header data.
 */
static struct zip_file_hdr ICACHE_FLASH_ATTR *zip_load_header(unsigned int address)
{
    struct zip_file_hdr *file_hdr = db_malloc(sizeof(struct zip_file_hdr), "file_hdr");
    uint32_t signature;
    unsigned int offset = address;
    
    debug("Loading header at 0x%x.\n", offset);
    
    //Test the signature
    if (!aflash_read(&signature, offset, sizeof(signature)))
    {
        debug("Could not read ZIP file header signature at %d.\n", offset);
        db_free(file_hdr);
        return(NULL);
    }
    if (signature != ZIP_FILE_HEADER_SIGNATURE)
    {
        debug("No more files, reading the ZIP file header signature at %d.\n", offset);
        db_free(file_hdr);
        return(NULL);
    }
    debug(" Header signature: 0x%x.\n", signature);
    
    //Skip over signature and "version needed extract".
    offset += 6;
    //Read the flags, compression method, last modified time and date.
    if (!aflash_read(&file_hdr->flags, offset, sizeof(uint16_t) * 4))
    {
        error("Failed reading the ZIP flags at %d.\n", offset);
        db_free(file_hdr);
        return(NULL);
    }
    //Skip crc32, and compressed size
    offset += (sizeof(uint16_t) * 4) + 8;
    //Read uncompressed data size, file name length, and extra data length.
    if (!aflash_read(&file_hdr->uncompressed_size, offset, sizeof(uint32_t) * 2))
    {
        error("Failed reading the ZIP file size at %d.\n", offset);
        db_free(file_hdr);
        return(NULL);
    }
    //Alloc an extra byte for the \0 byte.
    file_hdr->filename = db_malloc(file_hdr->filename_len + 1, "file_hdr->filename");
    //Load the file name.
    if (!aflash_read(file_hdr->filename, 
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
struct zip_file_hdr *zip_find_file_header(char *path) 
{
    struct zip_file_hdr *file_hdr;
    unsigned int offset = 0;
    size_t path_length;
    size_t filename_length;

    debug("Finding file header for path: %s.\n", path);
    
    //Zip file filenames don't have a leading /, so we strip it off.
    if (*path == '/') 
    {
        path++;
    }
    path_length = os_strlen(path);
    
    //Use look up if possible.
    if (zip_flut)
    {
		while ((offset < zip_flut_entries))
		{
			debug(" Trying file %s.\n", zip_flut[offset].filename);
			filename_length = os_strlen(zip_flut[offset].filename);			
			if ((!os_strncmp(zip_flut[offset].filename, path, filename_length)) &&
            (filename_length == path_length))
			{
				debug(" Found file in flut at %d.\n", offset);
				file_hdr = zip_load_header(zip_flut[offset].hdr_offset);
				if (!file_hdr)
				{
					error("Could not read file header for: %s.\n", path);
					return(NULL);
				}
				file_hdr->data_pos = zip_flut[offset].data_offset;
				return(file_hdr);
			}
			offset++;
		}
	}
	else
	{
		debug(" No flut.\n");
	
		offset = 0;
		file_hdr = zip_load_header(0);
		if (!file_hdr)
		{
			return(NULL);
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
			
			if ((!os_strncmp(file_hdr->filename, path, file_hdr->filename_len)) &&
				(file_hdr->filename_len == path_length))
			{
				//We found a match .
				debug("Found.\n");
				file_hdr->data_pos = offset;
				return(file_hdr);
			}
			
			//Skip to next header.
			offset += file_hdr->uncompressed_size;
			
			//Free the memory again
			zip_free_header(file_hdr);
			
			//Read it.
			file_hdr = zip_load_header(offset);
		}
    }

    debug("Not found!\n"); 
    return(NULL);
}

/**
 * @brief Check if the path is a directory.
 * 
 * @param path Path to check.
 * @return True if path is a directory.
 */
bool zip_is_dir(char *path) 
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
        file_hdr->filename[filename_len - 1] == '/') 
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
	debug("Freeing zip header (%p).\n", file_hdr);
	
	if (file_hdr)
	{
		if (file_hdr->filename)
		{
			db_free(file_hdr->filename);
		}
		if (file_hdr)
		{
			db_free(file_hdr);
		}
	}
	else
	{
		debug(" Already empty.\n");
	}
}

/**
 * @brief Initialise the zip routines.
 */
void ICACHE_FLASH_ATTR init_zip(void)
{
	struct zip_file_hdr *file_hdr;
    unsigned int offset = 0;
    unsigned int data_offset;
    unsigned short files = 0;

    debug("Initialising ZIP support.\n");

	debug(" Counting files.\n");
	file_hdr = zip_load_header(0);
    //Run while there are more headers.
    while (file_hdr) 
    {
        //Test bit 3 of the flags, to see if a data descriptor is used.
        if (!(file_hdr->flags & ( 1 << 2)))
        {
			//Ignore directories.
			if (file_hdr->filename[file_hdr->filename_len - 1] != '/')
			{
				files++;
			}
		}
		else
        {
            warn("\nZIP data descriptors are not supported.\n");
        }
        //Calculate the position of the file data.
        offset += ZIP_REAL_FILE_HEADER_SIZE;
        offset += file_hdr->filename_len;
        offset += file_hdr->extra_len;
		//Skip to next header.
        offset += file_hdr->uncompressed_size;                
        
        //Free the memory again
        zip_free_header(file_hdr);
        
        //Read it.
        file_hdr = zip_load_header(offset);  
    }
    debug(" %d files.\n", files); 
	zip_flut_entries = files;
	zip_flut = db_malloc(sizeof(struct zip_flut_entry) * files, "zip_flut init_zip");
	
	debug(" Adding files to file look up table.\n");
	files = 0;
	offset = 0;
	file_hdr = zip_load_header(0);
    //Run while there are more headers.
    while (file_hdr) 
    {
		data_offset = offset + ZIP_REAL_FILE_HEADER_SIZE + file_hdr->filename_len + file_hdr->extra_len;
        //Test bit 3 of the flags, to see if a data descriptor is used.
        if (!(file_hdr->flags & ( 1 << 2)))
        {
			//Ignore directories.
			if (file_hdr->filename[file_hdr->filename_len - 1] != '/')
			{
				debug(" Adding \"%s\" at 0x%x.\n", file_hdr->filename, offset);
				zip_flut[files].filename = db_malloc(os_strlen(file_hdr->filename) + 1, "zip_flut[files].filename init_zip.");
				os_strcpy(zip_flut[files].filename, file_hdr->filename);
				zip_flut[files].hdr_offset = offset;
				zip_flut[files].data_offset = data_offset;
				files++;
			}
		}
        offset = data_offset;
		//Skip to next header.
        offset += file_hdr->uncompressed_size;                
        
        //Free the memory again
        zip_free_header(file_hdr);
        
        //Read it.
        file_hdr = zip_load_header(offset);  
    }
}
