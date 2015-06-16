
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
 * @brief Structure to keep track of our progress.
 */
struct sm_http_fs_context
{
	/**
	 * @brief The name of the file to get the response from.
	 */
	char *filename;
	/**
	 * @brief The number of bytes that has been sent.
	 */
	size_t transferred;
};

/**
 * @brief Tell if we will handle a certain URI.
 * 
 * @param request The request to handle.
 * @return True if we can handle the URI.
 */
bool ICACHE_FLASH_ATTR http_fs_test(struct http_request *request)
{
	char *uri = request->uri;
	char *fs_uri = NULL;
	size_t uri_size;
	size_t fs_uri_size;
	size_t root_size = 0;
	size_t index_size = 10;
	FS_FILE_H file;
	struct sm_http_fs_context *context;
	
	//Check if we've already done this.
	if (!request->response.context)
	{
		//Skip first slash;
		uri++;
		//Change an '/' uri into 'index.html'.
		uri_size = os_strlen(uri);
		//If the last character before the zero byte is a slash, add index.html.
		if ((uri[uri_size - 1] != '/') && (uri[uri_size] == '\0'))
		{
			index_size = 0;		
		}

		//Check if we have a document root path.
		if (http_fs_doc_root)
		{
			root_size = os_strlen(http_fs_doc_root);
		}
		
		//Get size and mem.
		fs_uri_size = root_size + uri_size + index_size;
		fs_uri = db_malloc(fs_uri_size, "fs_uri http_fs_test");
		
		if (root_size)
		{
			debug(" Adding root %s.\n", http_fs_doc_root);
			os_memcpy(fs_uri, http_fs_doc_root, root_size);
		}
		
		os_memcpy(fs_uri + root_size, uri, uri_size);
		
		if (index_size)
		{
			debug(" Adding index.html.\n");
			os_memcpy(fs_uri + root_size + uri_size, "index.html", 10);
		}
		fs_uri[fs_uri_size] = '\0';	
		
		//Save file name in context.
		context = db_malloc(sizeof(struct sm_http_fs_context), "context http_fs_test");
		context->filename = fs_uri;
		request->response.context = context;
	}
	else
	{
		//The file name has all ready been saved, by a previous run.
		context = request->response.context;
	}
	
	//Try to open the URI as a file.
    file = fs_open(context->filename);
   	if (file > FS_EOF)
	{
		debug("File system handler found: %s.\n", context->filename);
		fs_close(file);
		
		return(true);
	}
	debug("No file system handler found.\n");
    return(false);  
}

/**
 * @brief Generate the response for a HEAD request from a file.
 * 
 * @param request Request that got us here.
 * @return Return unused.
 */
size_t ICACHE_FLASH_ATTR http_fs_head_handler(struct http_request *request)
{
	struct sm_http_fs_context *context = request->response.context;
	
	debug("File system HEAD handler handling %s.\n", context->filename);
	
	switch(request->response.state)
	{
		case HTTP_STATE_STATUS: send_status_line(request->connection, 
												 request->response.status_code);
								
								//Onwards
								request->response.state = HTTP_STATE_HEADERS;
								break;
		case HTTP_STATE_HEADERS: //Always send connections close and server info.
								 http_send_header(request->connection, 
												  "Connection",
											      "close");
								 http_send_header(request->connection,
												  "Server",
												  HTTP_SERVER_NAME);
								 //On to the message.
								 request->response.state = HTTP_STATE_MESSAGE;
								 break;
	}
	
    return(0);
}

/**
 * @brief Generate the response for a GET request from a file.
 * 
 * @param request Request that got us here.
 * @return Return the size of the response message in bytes.
 */
size_t ICACHE_FLASH_ATTR http_fs_get_handler(struct http_request *request)
{
	struct sm_http_fs_context *context = request->response.context;
	
	debug("File system GET handler handling %s.\n", context->filename);
	
	//Don't duplicate, just call the head handler.
	if ((request->response.state != HTTP_STATE_MESSAGE) ||
	    (request->response.state != HTTP_STATE_MESSAGE))
	{
		http_fs_head_handler(request);
	}
	
    return(0);
}

/**
 * @brief Deallocate memory used for the response.
 * 
 * @param request Request for which to free the response.
 */
void ICACHE_FLASH_ATTR http_fs_destroy(struct http_request *request)
{
	if (request->response.context)
	{
		if (((struct sm_http_fs_context *)request->response.context)->filename)
		{
			db_free(((struct sm_http_fs_context *)request->response.context)->filename);
		}
		db_free(request->response.context);
	}
}
