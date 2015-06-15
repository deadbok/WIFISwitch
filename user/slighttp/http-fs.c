
/**
 * @file http-fs.c
 *
 * @brief Interface between the HTTP server, and the file system.
 * 
 * This works like any other response generator, and 
 * 
 * @copyright
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@@ace2>
 * 
 * @license
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
#include "eagle_soc.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_config.h"
#include "fs/fs.h"
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "slighttp/http-response.h"

/**
 * @brief Tell if we will handle a certain URI.
 * 
 * @param uri The URI to test.
 * @return True if we can handle the URI.
 */
bool ICACHE_FLASH_ATTR http_fs_test(char *uri)
{
	char *fs_uri = NULL;
	size_t uri_size;
	size_t fs_uri_size;
	size_t doc_root_size;
	bool add_index = false;
	FS_FILE_H file;
	
	//Skip first slash;
	uri++;
	//Change an '/' uri into '/index.html'.
	uri_size = os_strlen(uri);
	//If the last character before the zero byte is a slash, add index.html.
	if ((uri[uri_size - 1] == '/') && (uri[uri_size] == '\0'))
	{
		add_index = true;		
	}
	else
	{
		add_index = false;
	}

	//Add document root to file system path.
	//Check if we have a document root path.
	if (http_fs_doc_root)
	{
		debug(" Adding root %s.\n", http_fs_doc_root);
		//Document root is non emtpy, find its length.
		doc_root_size = os_strlen(http_fs_doc_root);
		fs_uri_size = uri_size + doc_root_size;
		if (add_index)
		{
			fs_uri_size += 10;
		}
		debug("Sizes: uri %d, doc_root %d, fs_uri %d.\n", uri_size, doc_root_size, fs_uri_size); 
			
		//Get mem.
		fs_uri = db_malloc(fs_uri_size, "fs_uri http_fs_test");
		//Assemble the path.
		os_memcpy(fs_uri, http_fs_doc_root, doc_root_size);
		os_memcpy(fs_uri + doc_root_size, uri, uri_size);
		if (add_index)
		{
			debug(" Adding index.html.\n");
			os_memcpy(fs_uri + doc_root_size + uri_size, "index.html", 10);
		}
		fs_uri[fs_uri_size] = '\0';
	}
	
	debug("FS checking for %s.\n", fs_uri);	
	//Try to open the URI as a file.
    file = fs_open(fs_uri);
   	if (file > FS_EOF)
	{
		debug("FS handler found: %s.\n", fs_uri);
		fs_close(file);
		return(true);
	}
    return(false);  
}

/**
 * @brief Generate the HTML for configuring the network connection.
 * 
 * @param uri The URI to answer.
 * @param request Data for the request that got us here.
 * @return The HTML.
 */
size_t ICACHE_FLASH_ATTR http_fs_handler(struct http_request *request)
{

    return(0);
}

/**
 * @brief Deallocate memory used for the HTML.
 * 
 * @param html A pointer to the mem to deallocate.
 */
void ICACHE_FLASH_ATTR http_fs_destroy(void)
{
}
