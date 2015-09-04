
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
#include "tools/strxtra.h"
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "slighttp/http-response.h"
#include "handlers/fs/http-fs.h"

bool http_fs_test(struct http_request *request);
void http_fs_header(struct http_request *request, char *header_line);
signed int http_fs_head_handler(struct http_request *request);
signed int http_fs_get_handler(struct http_request *request);
void http_fs_destroy(struct http_request *request);

/**
 * @brief Struct used to register the handler.
 */
struct http_response_handler http_fs_handler =
{
	http_fs_test,
	http_fs_header,
	{
		NULL,
		http_fs_get_handler,
		http_fs_head_handler,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	}, 
	http_fs_destroy
};

/**
 * @brief Root to use when searching the fs.
 */
static char *http_fs_root = NULL;
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
 * @brief Initialise the file system handler.
 * 
 * @param root Directory to use as root for serving files.
 * @return True on success.
 */
bool ICACHE_FLASH_ATTR http_fs_init(char *root)
{
	debug("Initialising file system support using %s.\n", root);
	if (!root)
	{
		error("Root is empty.\n");
		return(false);
	}
	if (http_fs_root )
	{
		error("Root already set.\n");
		return(false);
	}
	http_fs_root = db_malloc(sizeof(char) * os_strlen(root) + 1, "http_fs_root http_fs_init");
	os_memcpy(http_fs_root, root, os_strlen(root) + 1);
	
	return(true);
}

/**
 * @brief Tell if we will handle a certain URI.
 * 
 * @param request The request to handle.
 * @return True if we can handle the URI.
 */
bool ICACHE_FLASH_ATTR http_fs_test(struct http_request *request)
{
	//Handle anything, but might return 404.
    return(true); 
}

/**
 * @brief Handle headers.
 * 
 * @param request The request to handle.
 * @param header_line Header line to handle.
 */
void ICACHE_FLASH_ATTR http_fs_header(struct http_request *request, char *header_line)
{
	debug("HTTP FS header handler.\n");
}

/**
 * @brief Open a file for a request.
 * 
 * Populates the context.
 * 
 * @param request The request.
 * @return True on success.
 */
 bool ICACHE_FLASH_ATTR http_fs_open_file(struct http_request *request)
{
	char *uri = request->uri;
	char *fs_uri = NULL;
	size_t uri_size;
	size_t fs_uri_size;
	size_t root_size = 0;
	size_t index_size = 10;
	struct http_fs_context *context;
	
	debug("HTTP FS handler looking for %s.\n", uri);
	//Check if we've already done this.
	if (!request->response.context)
	{
		//Return an error page, if status says so.
		if (request->response.status_code > 399)
		{
			debug(" Error status %d.\n", request->response.status_code);
			uri = db_malloc(sizeof(char) * 10, "uri http_fs_test");
			uri[0] = '/';
			itoa(request->response.status_code, uri + 1, 10);
			os_memcpy(uri + 4, ".html\0", 6);
			debug(" Status not 200 using URI %s.\n", uri);
		}	
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
		if (http_fs_root)
		{
			root_size = os_strlen(http_fs_root);
		}
		
		//Get size and mem.
		fs_uri_size = root_size + uri_size + index_size;
		fs_uri = db_malloc(fs_uri_size, "fs_uri http_fs_test");
		
		if (root_size)
		{
			debug(" Adding root %s.\n", http_fs_root);
			os_memcpy(fs_uri, http_fs_root, root_size);
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
		
		//Free URI if needed.
		if (request->response.status_code > 399)
		{
			db_free(--uri);
		}
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
	//Free data since no one should need them.
	http_fs_destroy(request);
	debug("No file system handler found.\n");
    return(false);
}

/**
 * @brief Generate the response for a HEAD request from a file.
 * 
 * @param request Request that got us here.
 * @return Return unused.
 */
signed int ICACHE_FLASH_ATTR http_fs_head_handler(struct http_request *request)
{
	struct http_fs_context *context = request->response.context;
	char str_size[16];
	unsigned char i;
	size_t ext_size;
	char *ext;
	unsigned short ret = 0;
	static bool done = false;
	
	//If the send buffer is over 200 bytes, this should never fill it.
	debug("File system HEAD handler.\n");
	if (done)
	{
		debug(" State done.\n");
	}
		
	switch(request->response.state)
	{
		case HTTP_STATE_STATUS:
			//Go on if we're done.
			if (done)
			{
				done = false;
				ret = 0;
				request->response.state++;
				break;
			}
			if (!http_fs_open_file(request))
			{
				/*If this is an error URI, there is
				*no reason to go looking for it again. */
				if (request->response.status_code < 399)
				{
					request->response.status_code = 404;
					debug("File not found, trying for 404 page.\n");
					if (!http_fs_open_file(request))
					{
						//No error page, drop the ball.
						request->response.state = HTTP_STATE_ERROR;
						return(0);
					}
				}
				else
				{
					//No error page, drop the ball.
					request->response.state = HTTP_STATE_ERROR;
					return(0);
				}
			}
			ret = http_send_status_line(request->connection, request->response.status_code);
			//Onwards
			done = true;
			break;
		case HTTP_STATE_HEADERS: 
			//Go on if we're done.
			if (done)
			{
				//Stop if only HEAD was requested.
				if (request->type == HTTP_HEAD)
				{
					request->response.state = HTTP_STATE_ASSEMBLED;
				}
				else
				{
					request->response.state = HTTP_STATE_MESSAGE;
				}
				done = false;
				ret = 0;
				break;
			}
			//Always send connections close and server info.
			ret = http_send_header(request->connection, "Connection",
								   "close");
			ret += http_send_header(request->connection, "Server",
									HTTP_SERVER_NAME);
			//Get data size.
			context->total_size = fs_size(context->file);
			os_sprintf(str_size, "%d", context->total_size);
			//Send message length.
			ret += http_send_header(request->connection, 
									"Content-Length",
									str_size);
			//Find the MIME-type
			for (i = 0; i < HTTP_N_MIME_TYPES; i++)
			{
				ext_size = os_strlen(http_mime_types[i].ext);
				ext  = context->filename + os_strlen(context->filename) - ext_size;
				if (os_strncmp(http_mime_types[i].ext, 
							   ext,
							   ext_size) == 0)
				{
					break;
				}
			}
			if (i >= HTTP_N_MIME_TYPES)
			{
				warn(" Did not find a usable MIME type, using application/octet-stream.\n");
				ret += http_send_header(request->connection, "Content-Type", "application/octet-stream");
			}
			else
			{
				ret += http_send_header(request->connection, "Content-Type", http_mime_types[i].type);
			}
			//Send end of headers.
			ret += http_send(request->connection, "\r\n", 2);
			done = true;
			break;
	}
    return(ret);
}

/**
 * @brief Generate the response for a GET request from a file.
 * 
 * @param request Request that got us here.
 * @return Return the size of the response message in bytes.
 */
signed int ICACHE_FLASH_ATTR http_fs_get_handler(struct http_request *request)
{
	struct http_fs_context *context = request->response.context;
	char buffer[HTTP_FILE_CHUNK_SIZE];
	size_t data_left;
	size_t bytes = 0;
	size_t ret = 0;
	size_t buffer_free;
	static bool done = false;
	
	debug("File system GET handler.\n");
	if (done)
	{
		debug(" State done.\n");
	}
	
	//Don't duplicate, just call the head handler.
	if (request->response.state < HTTP_STATE_MESSAGE)
	{
		ret = http_fs_head_handler(request);
	}
	else
	{
		if (request->response.state == HTTP_STATE_MESSAGE)
		{
			//Go on if we're done.
			if (done)
			{
				done = false;
				request->response.state++;
				return(0);
			}
			data_left = context->total_size - context->transferred;
			buffer_free = HTTP_SEND_BUFFER_SIZE - (request->response.send_buffer_pos - request->response.send_buffer);
			//Read a chunk of data and send it.
			if (data_left > HTTP_FILE_CHUNK_SIZE)
			{
				//There is still more than HTTP_FILE_CHUNK_SIZE to read.
				if (buffer_free < HTTP_FILE_CHUNK_SIZE)
				{
					bytes = buffer_free;
					debug(" Truncating read to match send buffer space of %d bytes.\n", bytes);
				}
				else
				{
					bytes = HTTP_FILE_CHUNK_SIZE;
				}
			}
			else
			{
				//Last block.
				if (buffer_free < data_left)
				{
					bytes = buffer_free;
					debug(" Truncating read to match send buffer space of %d bytes.\n", bytes);
				}
				else
				{
					bytes = data_left;
				}
			}
			fs_read(buffer, bytes, sizeof(char), context->file);
			ret = http_send(request->connection, buffer, bytes);
			request->response.message_size += ret;
			context->transferred += ret;
			
			if ((context->total_size - context->transferred) == 0)
			{
				//We're done sending the message.
				fs_close(context->file);
				done = true;
			}
		}
	}
    return(ret);
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
			debug("Deallocating file name %s.\n", ((struct http_fs_context *)request->response.context)->filename);
			db_free(((struct http_fs_context *)request->response.context)->filename);
		}
		debug("Deallocating request handler context.\n");
		db_free(request->response.context);
		request->response.context = NULL;
	}
}
