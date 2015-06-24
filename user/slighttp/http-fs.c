
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
struct http_fs_context
{
	/**
	 * @brief The name of the file to get the response from.
	 */
	char *filename;
	/**
	 * @brief The number of bytes that has been sent.
	 */
	size_t transferred;
	/**
	 * @brief The total number of bytes to send.
	 */
	size_t total_size;
	/**
	 * @brief The file-
	 */
	FS_FILE_H file;
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
	struct http_fs_context *context;
	
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
		context = db_malloc(sizeof(struct http_fs_context), "context http_fs_test");
		context->filename = fs_uri;
		request->response.context = context;
	}
	else
	{
		//The file name has all ready been saved, by a previous run.
		context = request->response.context;
	}
	
	//Try to open the URI as a file.
    context->file = fs_open(context->filename);
   	if (context->file > FS_EOF)
	{
		debug("File system handler found: %s.\n", context->filename);
		context->transferred = 0;		
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
	struct http_fs_context *context = request->response.context;
	char str_size[16];
	unsigned char i;
	size_t ext_size;
	
	//If the send buffer is over 200 bytes, this should never fill it.
	debug("File system HEAD handler handling %s.\n", context->filename);
	
	switch(request->response.state)
	{
		case HTTP_STATE_STATUS:  http_send_status_line(request->connection, request->response.status_code);
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
								 //Get data size.
								 context->total_size = fs_size(context->file);
								 os_sprintf(str_size, "%d", context->total_size);
								 //Send message length.
								 http_send_header(request->connection, 
												  "Content-Length",
											      str_size);
								 //Find the MIME-type
								 for (i = 0; i < HTTP_N_MIME_TYPES; i++)
								 {
									 ext_size = os_strlen(http_mime_types[i].ext);
									 if (os_strncmp(http_mime_types[i].ext, 
									                context->filename + os_strlen(context->filename) - ext_size,
									                ext_size)
									     == 0)
									 {
										 break;
									 }
								 }
								 http_send_header(request->connection, "Content-Type", http_mime_types[i].type);	
								 //Send end of headers.
								 http_send(request->connection, "\r\n", 2);
								 //Stop if only HEAD was requested.
								 if (request->type == HTTP_HEAD)
								 {
									 request->response.state = HTTP_STATE_ASSEMBLED;
								 }
								 else
								 {
									 request->response.state = HTTP_STATE_MESSAGE;
								 }
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
	struct http_fs_context *context = request->response.context;
	char buffer[HTTP_FILE_CHUNK_SIZE];
	size_t data_left;
	
	debug("File system GET handler handling %s.\n", context->filename);
	
	//Don't duplicate, just call the head handler.
	if (request->response.state < HTTP_STATE_MESSAGE)
	{
		http_fs_head_handler(request);
	}
	else
	{
		if (request->response.state == HTTP_STATE_MESSAGE)
		{
			data_left = context->total_size - context->transferred;
			//Read a chunk of data and send it.
			if (data_left > HTTP_FILE_CHUNK_SIZE)
			{
				//There is still more than HTTP_FILE_CHUNK_SIZE to read.
				fs_read(buffer, HTTP_FILE_CHUNK_SIZE, sizeof(char), context->file);
				http_send(request->connection, buffer, HTTP_FILE_CHUNK_SIZE);
				context->transferred += HTTP_FILE_CHUNK_SIZE;
			}
			else
			{
				//Last block.
				fs_read(buffer, data_left, sizeof(char), context->file);
				http_send(request->connection, buffer, data_left);
				context->transferred += data_left;
				//We're done sending the message.
				fs_close(context->file);
				request->response.state = HTTP_STATE_ASSEMBLED;
			}
		}
	}

    return(context->total_size);
}

/**
 * @brief Deallocate memory used for the response.
 * 
 * @param request Request for which to free the response.
 */
void ICACHE_FLASH_ATTR http_fs_destroy(struct http_request *request)
{
	debug("Freeing data for file response.\n");
	if (request->response.context)
	{
		if (((struct http_fs_context *)request->response.context)->filename)
		{
			db_free(((struct http_fs_context *)request->response.context)->filename);
		}
		db_free(request->response.context);
		request->response.context = NULL;
	}
}
