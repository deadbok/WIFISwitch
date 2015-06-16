/** @file http-response.c
 *
 * @brief Dealing with responses for the HTTP server.
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
#include "tools/missing_dec.h"
#include "c_types.h"
#include "osapi.h"
#include "user_config.h"
#include "net/tcp.h"
#include "fs/fs.h"
#include "http-common.h"
#include "http-mime.h"
#include "http.h"
#include "http-response.h"

/**
 * @brief 200 status-line.
 */
char *http_status_line_200 = HTTP_STATUS_200;
/**
 * @brief 400 status-line.
 */
char *http_status_line_400 = HTTP_STATUS_400;
/**
 * @brief 404 status-line.
 */
char *http_status_line_404 = HTTP_STATUS_404;
/**
 * @brief 501 status-line.
 */
char *http_status_line_501 = HTTP_STATUS_501;

/**
 * @brief Send a header.
 * 
 * @param connection Pointer to the connection to use.
 * @param name The name of the header.
 * @param value The value of the header.
 */
void ICACHE_FLASH_ATTR http_send_header(struct tcp_connection *connection, char *name, char *value)
{
	char header[512];
	size_t size;

	debug("Sending header (%s: %s).\n", name, value);
	size = os_sprintf(header, "%s: %s\r\n", name, value);
	tcp_send(connection, header, size);
}

/**
 * @brief Send HTTP response status line.
 * 
 * @param connection Pointer to the connection to use. 
 * @param code Status code to use in the status line.
 */
void ICACHE_FLASH_ATTR send_status_line(struct tcp_connection *connection, unsigned short status_code)
{
	debug("Sending status line with status code %d.\n", status_code);
	
	switch (status_code)
	{
		case 200: tcp_send(connection, http_status_line_200, os_strlen(http_status_line_200));
				  break;
		case 400: tcp_send(connection, http_status_line_400, os_strlen(http_status_line_400));
				  break;
		case 404: tcp_send(connection, http_status_line_404, os_strlen(http_status_line_404));
				  break;
		case 501: tcp_send(connection, http_status_line_501, os_strlen(http_status_line_501));
				  break;
		default:  warn(" Unknown response code: %d.\n", status_code);
				  tcp_send(connection, http_status_line_501, os_strlen(http_status_line_501));
				  break;
	}
}

/**
 * @brief Tell if the server knows an URI.
 * 
 * @return Pointer to a handler struct, or NULL if not found.
 */
struct http_response_handler ICACHE_FLASH_ATTR *http_get_handlers(struct http_request *request)
{
	unsigned short i;
	
	//Check URI, go through and stop if URIs match.
	for (i = 0; i < n_response_handlers; i++)
	{
		debug("Trying handler %d.\n", i);
		if (response_handlers[i].will_handle(request))
		{
			debug("URI handlers for %s at %p.\n", request->uri, response_handlers + i);
			return(response_handlers + i);
		}
	}
	debug("No response handler found for URI %s.\n", request->uri);
	return(NULL);
} 

/**
 * @brief Send a HTTP response.
 * 
 * @param connection Pointer to the connection used.
 * @param status_code Status code of the response.
 */
void ICACHE_FLASH_ATTR http_send_response(
									struct tcp_connection *connection,
									unsigned short status_code)
{
    struct http_request *request = connection->free;
    unsigned short i;
    unsigned short handler_index;
    FS_FILE_H file;
    size_t msg_size = 0;
    char *error_msg;
    char *uri = NULL;
    char *fs_uri = NULL;
    char buffer[HTTP_FILE_CHUNK_SIZE];
    size_t uri_size, doc_root_size, ext_size;
    bool found = false;
    
    debug("Generate HTTP response.\n");
    
    //If there is an error, respond with an error page.
    if (status_code != 200)
    {
		uri = db_malloc(sizeof(char) * 10, "uri  http_generate_response");
		os_sprintf(uri, "/%d.html", status_code);
		fs_uri = uri;
		debug(" HTTP error response.\n");
	}
	else
	{
		//Change an '/' uri into '/index.html'.
		//Find the last '/'.
		for (i = 0; request->uri[i] != '\0'; i++)
		{
			if (request->uri[i] == '/')
			{
				uri = &request->uri[i];
			}
		}
		//Check if it is at the end of the uri.
		uri++;
		uri_size = os_strlen(request->uri);
		if (*uri == '\0')
		{
			debug(" Adding index.html.\n");
			//The uri ends on '/', add 'index.html'.
			uri = db_malloc(uri_size + 11, "uri http_generate_response");
			os_memcpy(uri, request->uri, uri_size);
			os_memcpy(uri + uri_size, "index.html\0", 11);
		}
		else
		{
			uri = request->uri;
		}
		fs_uri = uri;
		//Add document root to file system path.
		//Check if we have a document root path.
		if (http_fs_doc_root)
		{
			debug(" Adding root %s.\n", http_fs_doc_root);
			//Document root is non emtpy, find its length.
			doc_root_size = os_strlen(http_fs_doc_root);
			uri_size = os_strlen(uri);
			//Get mem.
			fs_uri = db_malloc(uri_size + doc_root_size, 
							"fs_uri http_generate_response");
			os_memcpy(fs_uri, http_fs_doc_root, doc_root_size);
			os_memcpy(fs_uri + doc_root_size, uri, uri_size);
			fs_uri[uri_size + doc_root_size] = '\0';
		}
	}

	debug(" Final static request URI: %s.\n", uri);
	debug(" Final file system request URI: %s.\n", fs_uri);
    
    //Try to open the URI as a file.
    file = fs_open(fs_uri);
   	if (file > FS_EOF)
	{
		//Found a file.
		found = true;
	}
	if (!found)
	{
		//Check in static uris, go through and stop if URIs match.
		for (i = 0; 
			 ((i < n_response_handlers) && (!response_handlers[i].will_handle(request))); 
			 i++);
		
		//Send response if URI is found
		if (i < n_response_handlers)
		{
			found = true;
			handler_index = i;
		}
	}

	if (found)
	{
		send_status_line(connection, status_code);
		
		//Always send connections close and server info.
		http_send_header(connection, "Connection", "close");
		http_send_header(connection, "Server", HTTP_SERVER_NAME);

		//Send file, if one was found.
		if (file > FS_EOF)
		{
			debug(" Found file: %s.\n", fs_uri);
			
			//Send the rest of the headers.
			//Find the MIME-type
			for (i = 0; i < HTTP_N_MIME_TYPES; i++)
			{
				ext_size = os_strlen(http_mime_types[i].ext);
				if (os_strncmp(http_mime_types[i].ext, fs_uri + os_strlen(fs_uri) - ext_size, ext_size) == 0)
				{
					break;
				}
			}
			http_send_header(connection, "Content-Type", http_mime_types[i].type);
			//Get the size of the message.
			msg_size = fs_size(file);
			os_sprintf(buffer, "%d", msg_size);
			http_send_header(connection, "Content-Length", buffer);
			tcp_send(connection, "\r\n", 2);
			
			//Only send data if not a HEAD request.		
			if (request->type != HTTP_HEAD)
			{
				i = msg_size;
				while (i > 0)
				{
					if (i > HTTP_FILE_CHUNK_SIZE)
					{
						//There is still more than HTTP_FILE_CHUNK_SIZE to read.
						fs_read(buffer, HTTP_FILE_CHUNK_SIZE, sizeof(char), file);
						tcp_send(connection, buffer, HTTP_FILE_CHUNK_SIZE);
						i -= HTTP_FILE_CHUNK_SIZE;
					}
					else
					{
						//Last block.
						fs_read(buffer, i, sizeof(char), file);
						tcp_send(connection, buffer, i);
						i = 0;
					}
				}
			}
			else
			{
				debug(" HEAD request, not sending data.\n");
			}
			fs_close(file);
		}
		else
		{
			//No file.
			//Call the handler.
			msg_size = response_handlers[handler_index].handlers[request->type - 1](request);
			//Clean up call back.
			if (response_handlers[handler_index].destroy)
			{
				response_handlers[handler_index].destroy(request);
			}
		}
	}
	else
	{
		//No page was found.
		warn(" No page was found.\n");
		if (status_code == 200)
		{
			status_code = 404;
		}
		
		send_status_line(connection, status_code);
		
		//Always send connections close and server info.
		http_send_header(connection, "Connection", "close");
		http_send_header(connection, "Server", HTTP_SERVER_NAME);
		http_send_header(connection, "Content-Type", http_mime_types[MIME_HTML].type);

		switch (status_code)
		{
			case 400: error_msg = HTTP_400_HTML;
					  msg_size = HTTP_400_HTML_LENGTH;
					  break;
			case 404: error_msg = HTTP_404_HTML;
					  msg_size = HTTP_404_HTML_LENGTH;
					  break;
			case 501: error_msg = HTTP_501_HTML;
					  msg_size = HTTP_501_HTML_LENGTH;
					  break;
			default:  warn("Unknown response code: %d.\n", status_code);
					  error_msg = HTTP_501_HTML;
					  msg_size = HTTP_501_HTML_LENGTH;
					  break;
		}
		os_sprintf(buffer, "%d", msg_size);
		http_send_header(connection, "Content-Length", buffer);
		tcp_send(connection, "\r\n", 2);
		
		tcp_send(connection, error_msg, msg_size);
	}
		
    //Print CLF output.
	os_sprintf(buffer, "%d", status_code);
    print_clf_status(connection, buffer, msg_size);
    
    //Close connection, now that we've threatened to do so.
    tcp_disconnect(connection);
	
	//Dealloc uri mem, if it has been malloced.
	if (fs_uri != uri)
	{
		db_free(fs_uri);
	}
	if (uri != request->uri)
	{
		db_free(uri);
	}
}
