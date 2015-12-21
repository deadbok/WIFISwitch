
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
#include "osapi.h"
#include "c_types.h"
#include "eagle_soc.h"
#include "user_interface.h"
#include "user_config.h"
#include "fs/fs.h"
#include "tools/strxtra.h"
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "handlers/http/fs/http-fs.h"
#include "slighttp/http-handler.h"
#include "slighttp/http-response.h"

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
	 * @brief The total number of bytes to send.
	 */
	size_t total_size;
	/**
	 * @brief The file.
	 */
	FS_FILE_H file;
};

/**
 * @brief Initialise the file system handler.
 * 
 * @param root Directory to use as root for serving files.
 * @return True on success.
 */
bool http_fs_init(char *root)
{
	debug("Initialising file system support using %s.\n", root);
	if (!root)
	{
		error("Root is empty.\n");
		return(false);
	}
	if (http_fs_root)
	{
		db_free(http_fs_root);
	}
	http_fs_root = db_malloc(sizeof(char) * os_strlen(root) + 1, "http_fs_root http_fs_init");
	os_memcpy(http_fs_root, root, os_strlen(root) + 1);
	
	return(true);
}

/**
 * @brief Open a file for a request.
 * 
 * Allocates and populates the context file data if a file is found.
 * 
 * @param request The request.
 * @param err Handles only error statuses if `true`. 
 * @return True on success.
 */
static bool http_fs_open_file(struct http_request *request, bool err)
{
	char *uri = request->uri;
	char *fs_uri = NULL;
	size_t uri_size;
	size_t root_size = 0;
	size_t index_size = 10;
	struct http_fs_context *context;
	
	debug("HTTP file system handler looking for %s.\n", uri);
/*
 * The following is already checked by the caller:

	if ((!err) && (request->response.status_code > 399))
	{
		warn(" Not handling errors, and status is %d. Leaving.\n",
			  request->response.status_code);
	}
	if ((err) && (request->response.status_code < 400))
	{
		warn(" Handling errors, and status is %d. Leaving.\n",
			  request->response.status_code);
	}
*/
	if ((err) && (request->response.status_code > 399))
	{
		debug(" Error status %d.\n", request->response.status_code);
		uri = db_malloc(sizeof(char) * 10, "uri http_fs_open_file");
		uri[0] = '/';
		itoa(request->response.status_code, uri + 1, 10);
		os_memcpy(uri + 4, ".html\0", 6);
		debug(" Using URI %s.\n", uri);
	}

	//Check if we've already done this.
	if (!request->response.context)
	{
		//Skip first slash.
		uri_size = os_strlen(uri) - 1;
		uri++;	
		debug(" Raw URI length %d.\n", uri_size);
		/*Change an '/' uri into 'index.html' if the last character
		 * before the zero byte is a slash, add index.html.
		 */
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
		fs_uri = db_zalloc(root_size + uri_size + index_size + 1,
						   "fs_uri http_fs_open_file");
		
		if (root_size)
		{
			os_strcat(fs_uri, http_fs_root);
			debug(" Added root %s URI %s.\n", http_fs_root, fs_uri);
		}
		
		os_strcat(fs_uri + root_size, uri);
		
		if (index_size)
		{
			os_strcat(fs_uri + root_size + uri_size, "index.html");
			debug(" Added index.html URI %s.\n", fs_uri);
		}
		
		//Save file name in context.
		context = db_malloc(sizeof(struct http_fs_context), "context http_fs_open_file");
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
	
	//Try opening the URI as a file.
    context->file = fs_open(context->filename);
   	if (context->file > FS_EOF)
	{
		//Set size.
		context->total_size = fs_size(context->file);
		debug("File found: %s.\n", context->filename);
		return(true);
	}
	//Free context if there is an error, and we are not handling it.
	if (!err)
	{
		debug(" Error status, freeing context.\n");
		db_free(context);
		request->response.context = NULL;
	}
	debug("File not found: %s.\n", context->filename);
    return(false);
}

/**
 * @brief Send a file.
 * 
 * *This function is stupid, and does not do much checking, please make
 * sure that the supplied request is valid and sound.*
 * 
 * Check and headers should be over, when calling this.
 * 
 * @param request The request to respond to.
 * @return The size of what has been sent, or one of the 
 *         RESPONSE_DONE_* values.
 */
static signed int do_message(struct http_request *request, bool err)
{
	/* We are only called by nice responsible functions that would never
	 * pass a NULL pointer ;).
	 */
	struct http_fs_context *context = request->response.context;
	size_t data_left, buffer_free, bytes;
	char buffer[HTTP_FILE_CHUNK_SIZE];
	signed int ret = 0;
	char *ext;
	
	//Status and headers.
	if (request->response.state == HTTP_STATE_NONE)
	{
		if (!http_fs_open_file(request, err))
		{
			debug(" Could not find file.\n");
			return(RESPONSE_DONE_CONTINUE);
		}
		//Context has just been created.
		context = request->response.context;
		//We have not send anything.
		request->response.message_size = 0;
		//Get extension.
		ext = http_mime_get_ext(context->filename);
		//Send status and headers.
		ret += http_send_status_line(request->connection, request->response.status_code);
		ret += http_send_default_headers(request, context->total_size, ext);
		if (request->type == HTTP_HEAD)
		{
			request->response.state = HTTP_STATE_DONE;
			return(ret);
		}
		else
		{
			//Go on to sending the file.
			request->response.state = HTTP_STATE_MESSAGE;
		}
	}
	
	//Message.
	if (request->response.state == HTTP_STATE_MESSAGE)
	{
		data_left = context->total_size - request->response.message_size;
		debug(" Sending %s, %d bytes left.\n", context->filename, data_left);
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
		if (bytes)
		{
			fs_read(buffer, bytes, sizeof(char), context->file);
			ret += http_send(request->connection, buffer, bytes);
			request->response.message_size += bytes;
			//Might send status and header data as well.
			if (ret >= bytes)
			{
				return(ret);
			}
			warn(" Not all data was sent (message %d, sent %d bytes).\n", bytes, ret);
			return(ret);
		}
		else
		{
			//We're done sending the message.
			request->response.state = HTTP_STATE_DONE;
		}
	}
	
	//Free memory.
	if (request->response.state == HTTP_STATE_DONE)
	{
		if (ret)
		{
			warn("Did not sent full message.\n");
		}
		fs_close(context->file);
		debug("Freeing data for file response.\n");
		if (context)
		{
			if (context->filename)
			{
				debug("Deallocating file name %s.\n", context->filename);
				db_free(context->filename);
			}
			debug("Deallocating request handler context.\n");
			db_free(context);
			request->response.context = NULL;
		}
	}
	debug("Response done.\n");
	return(RESPONSE_DONE_FINAL);
}

/**
 * @brief Handle HTTP file system responses.
 * 
 * Does nothing if an error status is set.
 */
signed int http_fs_handler(struct http_request *request)
{	
	debug("Entering HTTP File system handler (request %p).\n", request);
	
	if (!request)
	{
		warn("Empty request.\n");
		return(RESPONSE_DONE_ERROR);
	}
	
	http_free_request_headers(request);
	//Skip on error.
	if (request->response.status_code > 399)
	{
		debug(" Error status %d, skipping.\n",
			  request->response.status_code);
		return(RESPONSE_DONE_CONTINUE);
	}
	if ((request->type != HTTP_GET) &&
		(request->type != HTTP_HEAD))
	{
		debug(" File system handler only supports GET.\n");
		return(RESPONSE_DONE_CONTINUE);
	}
	
	return(do_message(request, false));
}

/**
 * @brief Handle HTTP error responses from file system responses.
 * 
 * If called with an error status, the handler will search the for a
 * html file with the same name as the status code (e.g. 404.html).
 */
signed int http_fs_error_handler(struct http_request *request)
{
	debug("Entering HTTP error file system handler (request %p).\n",
		  request);
	if (!request)
	{
		warn("Empty request.\n");
		return(RESPONSE_DONE_ERROR);
	}
	
	http_free_request_headers(request);
	//Set error status is to error since we got here.
	if (request->response.status_code < 400)
	{
		debug(" No error status (%d), setting status 404.\n",
			  request->response.status_code);
		//If we got here, some one else has not responded.
		request->response.status_code = 404;
	}
	
	return(do_message(request, true));
}
