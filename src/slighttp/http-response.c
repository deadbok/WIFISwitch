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
#include "tools/strxtra.h"
#include "slighttp/http-common.h"
#include "slighttp/http-mime.h"
#include "slighttp/http.h"
#include "slighttp/http-request.h"
#include "slighttp/http-response.h"
#include "slighttp/http-tcp.h"
#include "slighttp/http-handler.h"

/**
 * @brief Pointer to a pointer to the connection.
 * 
 * Pointer to connection pointer, from the response queue.
 */
 void *connection_ptr = NULL;

/**
 * @brief Send HTTP response status line.
 * 
 * Handles 200, 204, 400, 403, 405, and 501.
 * 
 * @param connection Pointer to the connection to use. 
 * @param code Status code to use in the status line.
 * @return Size of data that has been sent.
 */
unsigned char http_send_status_line(struct net_connection *connection, unsigned short status_code)
{
	size_t size;
	char *response;
	char status_line[15];

	debug("Sending status line with status code %d.\n", status_code);	
	switch (status_code)
	{
		case 101:
			response = HTTP_STATUS_101;
			size = os_strlen(HTTP_STATUS_101);
			break;
		case 200:
			response = HTTP_STATUS_200;
			size = os_strlen(HTTP_STATUS_200);
			break;
		case 204: 
			response = HTTP_STATUS_204;
			size = os_strlen(HTTP_STATUS_204);
			break;
		case 400: 
			response = HTTP_STATUS_400;
			size = os_strlen(HTTP_STATUS_400);
			break;
		case 403: 
			response = HTTP_STATUS_403;
			size = os_strlen(HTTP_STATUS_403);
			break;
		case 404: 
			response = HTTP_STATUS_404;
			size = os_strlen(HTTP_STATUS_404);
			break;
		case 405: 
			response = HTTP_STATUS_405;
			size = os_strlen(HTTP_STATUS_405);
			break;
		case 426: 
			response = HTTP_STATUS_426;
			size = os_strlen(HTTP_STATUS_426);
			break;
		case 500: 
			response = HTTP_STATUS_500;
			size = os_strlen(HTTP_STATUS_500);
			break;
		case 501: 
			response = HTTP_STATUS_501;
			size = os_strlen(HTTP_STATUS_501);
			break;
		default:  
			debug(" Unknown response code: %d.\n", status_code);
			os_memcpy(status_line, HTTP_STATUS_HTTP_VERSION " ", 9);
			size = 9;
			itoa(status_code, status_line + size, 10);
			size += 3;
			memcpy(status_line + size, "\r\n\0", 3);
			size += 2;
			response = status_line;
			break;
	}
	return(http_send(connection, response, size));
}

/**
 * @brief Send a header.
 * 
 * @note Header name and value may be no larger than 508 bytes.
 * 
 * @param connection Pointer to the connection to use.
 * @param name The name of the header.
 * @param value The value of the header.
 * @return Size of data sent.
 */
unsigned short http_send_header(struct net_connection *connection, char *name, char *value)
{
	char header[512];
	char *temp_val = "";
	size_t size;

	debug("Sending header (%s: %s).\n", name, value);
	if (name)
	{
		if (value)
		{
			temp_val = value;
			if (os_strlen(name) + os_strlen(temp_val) > 508)
			{
				warn("Header to large to send.\n");
				return(0);
			}
		}
		else
		{
			warn("Header value is NULL.\n");
		}
		size = os_sprintf(header, "%s: %s\r\n", name, temp_val);
		return(http_send(connection, header, size));
	}
	else
	{
		warn("Header name is NULL.\n");
		return(0);
	}
}

/**
 * @brief Send web server default headers.
 * 
 * Send `Connection`, `Server`, `Content-Length`, and `Content-Type`.
 * 
 * @param request The request to respond to.
 * @param size Message size.
 * @param mime Mime type file extension.
 * @return Size of send data.
 */
signed int http_send_default_headers(
	struct http_request *request,
	size_t size,
	char *mime
)
{
	char str_size[16];
	unsigned int i;
	signed int ret;
	
	//Always send connections close and server info.
	ret = http_send_header(request->connection, "Connection",
						   "close");
	ret += http_send_header(request->connection, "Server",
							HTTP_SERVER_NAME);
	os_sprintf(str_size, "%d", size);
	//Send message length.
	ret += http_send_header(request->connection, 
							"Content-Length",
							str_size);
	//Find the MIME-type
	if (mime)
	{
		for (i = 0; i < HTTP_N_MIME_TYPES; i++)
		{
			//TODO: Maybe use os_strcmp to avoid mixing up htm and html.
			if (os_strcmp(http_mime_types[i].ext, mime) == 0)
			{
				break;
			}
		}
		if ((i >= HTTP_N_MIME_TYPES) || (!mime))
		{
			debug(" Did not find a usable MIME type, using application/octet-stream.\n");
			ret += http_send_header(request->connection, "Content-Type", "application/octet-stream");
		}
		else
		{
			ret += http_send_header(request->connection, "Content-Type", http_mime_types[i].type);
		}
	}

	//Send end of headers.
	ret += http_send(request->connection, "\r\n", 2);

	return(ret);
}

/**
 * @brief Buffer some data for sending via TCP.
 * 
 * @note Can as maximum send #HTTP_SEND_BUFFER_SIZE bytes.
 * 
 * @param connection A pointer to the connection to use to send the data.
 * @param data A pointer to the data to send.
 * @param size Size (in bytes) of the data to send.
 * @return Number of bytes buffered.
 */
size_t http_send(struct net_connection *connection, char *data, size_t size)
{
    struct http_request *request;
    size_t buffer_free;
    
    debug("Buffering %d bytes of TCP data (%p using %p),\n", size, data, connection);
	request = connection->user;
	
	buffer_free = HTTP_SEND_BUFFER_SIZE - (request->response.send_buffer_pos - request->response.send_buffer);
	if (buffer_free < size)
	{
		debug(" Send buffer to small for %d bytes, currently %d bytes free.\n", size, buffer_free);
		return(0);
	}
	os_memcpy(request->response.send_buffer_pos, data, size);
	request->response.send_buffer_pos += size;
	debug(" Buffer free %d.\n", buffer_free - size);
	
	return(size);
}

/**
 * @brief Send the waiting buffer.
 * 
 * @return true if nothing went wrong.
 */
static bool send_buffer(struct http_request *request)
{
	size_t buffer_use;

	debug("Sending buffer %p using connection %p.\n", request->response.send_buffer, request->connection);
	buffer_use = request->response.send_buffer_pos - request->response.send_buffer;
	
	if (buffer_use)
	{
		return(net_send(request->response.send_buffer, buffer_use, request->connection->conn));
	}
	debug( "Buffer empty.\n");
	return(true);
	
}

signed int http_handle_response(struct http_request *request)
{
	signed int ret = 0;
	
	debug("Handle response for request %p.\n", request);
	if (request->response.handler == NULL)
	{
		debug(" No handler, finding one.\n");
		//Get the first handler.
		request->response.handler = http_get_handler(request, NULL);
	}
	if (request->response.handler == NULL)
	{
		debug(" No handler found.\n");
		return(RESPONSE_DONE_ERROR);
	}
	//Handle while there are handlers.
	while (request->response.handler)
	{
		//Handle.
		debug(" Calling handler at %p.\n", request->response.handler);
		ret = request->response.handler(request);
		//Leave to sent callback if data has been sent.
		if (ret > 0)
		{
			debug(" Data has been buffered.\n");
			if (!send_buffer(request))
			{
				debug(" Couldn't send buffer.\n");
			}
			return(ret);
		}
		//Stop if handler says so.
		if (ret == RESPONSE_DONE_FINAL)
		{
			debug(" Handler is done and no new handler is to be called.\n");
			//Don't leave the user pointer dangling.
			request->connection->user = NULL;
			http_free_request(request);
			//Done sending, print log line.
			http_print_clf_status(request);
			return(RESPONSE_DONE_FINAL);
		}
		//Stop but do not clean up.
		if (ret == RESPONSE_DONE_NO_DEALLOC)
		{
			debug(" Handler is done, no new handler is to be called, connection and request data are kept.\n");
						//Done sending, print log line.
			http_print_clf_status(request);
			return(RESPONSE_DONE_NO_DEALLOC);
		}
		debug(" Handler is done, finding next handler.\n");
		//Find next handler.
		request->response.handler = http_get_handler(request, request->response.handler);
	}
	//Done sending, print log line.
	http_print_clf_status(request);
	return(RESPONSE_DONE_FINAL);
}
