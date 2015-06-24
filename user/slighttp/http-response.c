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
#include "http-request.h"
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
 * @note Header name and value may be no larger than 508 bytes.
 * 
 * @param connection Pointer to the connection to use.
 * @param name The name of the header.
 * @param value The value of the header.
 * @return 0 on failure, 1 on success.
 */
unsigned char ICACHE_FLASH_ATTR http_send_header(struct tcp_connection *connection, char *name, char *value)
{
	char header[512];
	size_t size;

	debug("Sending header (%s: %s).\n", name, value);
	size = os_sprintf(header, "%s: %s\r\n", name, value);
	return(http_send(connection, header, size));
}

/**
 * @brief Send HTTP response status line.
 * 
 * @param connection Pointer to the connection to use. 
 * @param code Status code to use in the status line.
 * @return 0 on failure, 1 on success.
 */
unsigned char ICACHE_FLASH_ATTR http_send_status_line(struct tcp_connection *connection, unsigned short status_code)
{
	unsigned char ret;
	debug("Sending status line with status code %d.\n", status_code);
	
	switch (status_code)
	{
		case 200: ret = http_send(connection, http_status_line_200, os_strlen(http_status_line_200));
				  break;
		case 400: ret = http_send(connection, http_status_line_400, os_strlen(http_status_line_400));
				  break;
		case 404: ret = http_send(connection, http_status_line_404, os_strlen(http_status_line_404));
				  break;
		case 501: ret = http_send(connection, http_status_line_501, os_strlen(http_status_line_501));
				  break;
		default:  warn(" Unknown response code: %d.\n", status_code);
				  ret = http_send(connection, http_status_line_501, os_strlen(http_status_line_501));
				  break;
	}
	return(ret);
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
 * @brief Buffer some data for sending via TCP.
 * 
 * @note Can as maximum send #HTTP_SEND_BUFFER_SIZE bytes.
 * 
 * @param connection A pointer to the connection to use to send the data.
 * @param data A pointer to the data to send.
 * @param size Size (in bytes) of the data to send.
 * @return 0 on failure, 1 on success.
 */
unsigned char ICACHE_FLASH_ATTR http_send(struct tcp_connection *connection, char *data, size_t size)
{
    struct http_request *request;
    size_t buffer_free;
    
    debug("Buffering %d bytes of TCP data (%p using %p),\n", size, data, connection);
	request = connection->free;
	if (!request->response.send_buffer)
	{
		request->response.send_buffer = db_malloc(HTTP_SEND_BUFFER_SIZE, "request->response.send_buffer http_send");
		request->response.send_buffer_pos = request->response.send_buffer;
	}
	
	buffer_free = HTTP_SEND_BUFFER_SIZE - (request->response.send_buffer_pos - request->response.send_buffer);
	if (buffer_free < size)
	{
		debug(" Send buffer to small for %d bytes, currently %d bytes free.\n", size, buffer_free);
		return(0);
	}
	os_memcpy(request->response.send_buffer_pos, data, size);
	request->response.send_buffer_pos += size;
	debug(" Buffer free %d.\n", buffer_free - size);
	
	return(1);
}

/**
 * @brief Send the waiting buffer.
 * 
 * @return true on success.
 */
 static bool send_buffer(struct tcp_connection *connection)
 {
	 struct http_request *request = connection->free;
	 size_t buffer_use;
	 
	 debug("Sending buffer %p using connection %p.\n", request->response.send_buffer, connection);
	 buffer_use = request->response.send_buffer_pos - request->response.send_buffer;
	 
	 return(tcp_send(connection, request->response.send_buffer, buffer_use));
 }	 

/**
 * @brief Process and send response data for a connection.
 * 
 * @param connection Pointer to connection to use.
 */
void ICACHE_FLASH_ATTR http_process_response(struct tcp_connection *connection)
{
	struct http_request *request = connection->free;
	
	debug("Processing request for connection %p.\n", connection);
	if (connection->sending)
	{
		debug("Still sending.\n");
		return;
	}
	//Since request was zalloced, request->type will be zero until parsed.
	if (request)
	{
		if (request->type != HTTP_NONE)
		{
			//Get started
			if (request->response.state == HTTP_STATE_NONE)
			{
				debug(" New response.\n");
				//Find someone to respond.
				request->response.state = HTTP_STATE_FIND;
			}
			switch (request->response.state)
			{
				case HTTP_STATE_FIND: request->response.handlers = http_get_handlers(request);
									  //No handler found.
									  if (!request->response.handlers->handlers[request->type - 1])
									  {
										  request->response.status_code = 404;
									  } 
									  //Next state.
									  request->response.state = HTTP_STATE_STATUS;
									  http_process_response(connection);
									  break;
				case HTTP_STATE_STATUS: //Send response
				case HTTP_STATE_HEADERS:
				case HTTP_STATE_MESSAGE: debug(" Calling response handler %p.\n",
											    request->response.handlers->handlers[request->type - 1]);
										 request->response.handlers->handlers[request->type - 1](request); 
										 send_buffer(connection);
										 break;
				case HTTP_STATE_ASSEMBLED: debug(" Waiting for message dispatch.\n"); 
										   break;
				case HTTP_STATE_DONE: if (!connection->sending)
									  {
										  debug("Closing connection %p.\n", connection);
										  tcp_disconnect(connection);
										  if (connection->free)
										  {
											  request->response.handlers->destroy(connection->free);
											  http_free_request(connection);
										  }
										  tcp_free(connection);
									  }
									  break;
			}
		}
	}
	else
	{
		debug("No request parsed yet.\n");
	}
}
