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
#include "http-common.h"
#include "http-mime.h"
#include "http.h"
#include "http-request.h"
#include "http-response.h"
#include "http-tcp.h"
#include "http-handler.h"

/**
 * @brief 200 status-line.
 */
char *http_status_line_200 = HTTP_STATUS_200;
/**
 * @brief 204 status-line.
 */
char *http_status_line_204 = HTTP_STATUS_204;
/**
 * @brief 400 status-line.
 */
char *http_status_line_400 = HTTP_STATUS_400;
/**
 * @brief 403 status-line.
 */
char *http_status_line_403 = HTTP_STATUS_403;
/**
 * @brief 404 status-line.
 */
char *http_status_line_404 = HTTP_STATUS_404;
/**
 * @brief 405 status-line.
 */
char *http_status_line_405 = HTTP_STATUS_405;
/**
 * @brief 500 status-line.
 */
char *http_status_line_500 = HTTP_STATUS_500;
/**
 * @brief 501 status-line.
 */
char *http_status_line_501 = HTTP_STATUS_501;

/**
 * @brief Pointer to a pointer to the connection.
 * 
 * Pointer to connection pointer, from the response queue.
 */
 void *connection_ptr = NULL;

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
unsigned short ICACHE_FLASH_ATTR http_send_header(struct tcp_connection *connection, char *name, char *value)
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
 * @return Size of data that has been sent.
 */
unsigned char ICACHE_FLASH_ATTR http_send_status_line(struct tcp_connection *connection, unsigned short status_code)
{
	size_t size;
	char *response;

	debug("Sending status line with status code %d.\n", status_code);	
	switch (status_code)
	{
		case 200: response = http_status_line_200;
				  size = os_strlen(http_status_line_200);
				  break;
		case 204: response = http_status_line_204;
				  size = os_strlen(http_status_line_204);
				  break;
		case 400: response = http_status_line_400;
				  size = os_strlen(http_status_line_400);
				  break;
		case 403: response = http_status_line_403;
				  size = os_strlen(http_status_line_403);
				  break;
		case 404: response = http_status_line_404;
				  size = os_strlen(http_status_line_404);
				  break;
		case 405: response = http_status_line_405;
				  size = os_strlen(http_status_line_405);
				  break;				  
		case 500: response = http_status_line_500;
				  size = os_strlen(http_status_line_500);
				  break;
		case 501: response = http_status_line_501;
				  size = os_strlen(http_status_line_501);
				  break;
		default:  warn(" Unknown response code: %d.\n", status_code);
				  response = http_status_line_501;
				  size = os_strlen(http_status_line_501);
				  break;
	}
	return(http_send(connection, response, size));
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
size_t ICACHE_FLASH_ATTR http_send(struct tcp_connection *connection, char *data, size_t size)
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
 * @return true on success.
 */
 static bool send_buffer(struct tcp_connection *connection)
 {
	 struct http_request *request = connection->user;
	 size_t buffer_use;
	 
	 debug("Sending buffer %p using connection %p.\n", request->response.send_buffer, connection);
	 buffer_use = request->response.send_buffer_pos - request->response.send_buffer;
	 
	 return(tcp_send(connection, request->response.send_buffer, buffer_use));
 }
 
/**
 * @brief Last chance error status handler.
 * 
 * @param connection Connection to use.
 * @param status_code The code to handle.
 */
void fall_through_status_handler(struct tcp_connection *connection, unsigned short status_code)
{
	size_t size = 0;
	char str_size[5];
	char *msg = NULL;

	debug("Last chance handler status code %d.\n", status_code);
	switch (status_code)
	{
		case 400: size = HTTP_400_HTML_LENGTH;
				  msg = HTTP_400_HTML;
				  break;
		case 403: size = HTTP_403_HTML_LENGTH;
				  msg = HTTP_403_HTML;
				  break;
		case 404: size = HTTP_404_HTML_LENGTH;
				  msg = HTTP_404_HTML;
				  break;
		case 405: size = HTTP_405_HTML_LENGTH;
				  msg = HTTP_405_HTML;
				  break;
		case 500: size = HTTP_500_HTML_LENGTH;
				  msg = HTTP_500_HTML;
				  break;
		case 501: size = HTTP_501_HTML_LENGTH;
				  msg = HTTP_501_HTML;
				  break;
	}
	itoa(size, str_size, 10);
	http_send_status_line(connection, status_code);
	http_send_header(connection, "Connection", "close");
	http_send_header(connection, "Server", HTTP_SERVER_NAME);	
	//Send message length.
	http_send_header(connection, "Content-Length", str_size);
	http_send_header(connection, "Content-Type", http_mime_types[MIME_HTML].type);	
	//Send end of headers.
	http_send(connection, "\r\n", 2);
	if (msg)
	{
		http_send(connection, msg, size);
	}
}

/**
 * @brief Process and send response data for a connection.
 * 
 * @param connection Pointer to connection to use.
 * @return True when done.
 */
void ICACHE_FLASH_ATTR http_process_response(struct tcp_connection *connection)
{
	struct http_request *request = connection->user;
	signed int size;
	
	debug("Processing request for connection %p.\n", connection);
	//Since request was zalloced, request->type will be zero until parsed.
	if (request)
	{
		//Increase recursion level.
		request->response.level++;
		debug(" -> Recursion level: %d.\n", request->response.level);
		debug(" Request type %d,\n", request->type);
		//Get started
		if (request->response.state == HTTP_STATE_NONE)
		{
			debug(" New response.\n");
			//Find someone to respond.
			request->response.state = HTTP_STATE_STATUS;
		}
		switch (request->response.state)
		{
			case HTTP_STATE_STATUS: 
			case HTTP_STATE_HEADERS: 
			case HTTP_STATE_MESSAGE: 
				//Send response, until there is no more to send
				if (request->response.handlers->method_cb[request->type -1])
				{
					debug(" Calling response handler %p.\n",
						  request->response.handlers->method_cb[request->type - 1]);
					size = request->response.handlers->method_cb[request->type - 1](request);
					if (size > 0)
					{
						debug(" Response %d bytes.\n", size); 
						if (request->response.send_buffer_pos > request->response.send_buffer)
						{
							debug(" Sending buffer contents.\n");
							send_buffer(connection);
						}
						else
						{
							http_process_response(connection);
						}
					}
					else if (size == 0)
					{
						if (request->response.send_buffer_pos > request->response.send_buffer)
						{
							debug(" Sending buffer contents.\n");
							send_buffer(connection);
						}
						http_process_response(connection);
					}
				}
				else
				{
					warn(" No handler, sending error status.\n");
					request->response.status_code = 501;
					request->response.state = HTTP_STATE_ERROR;
					http_process_response(connection);						
				}
				break;
			case HTTP_STATE_ASSEMBLED:
				if (request->response.send_buffer_pos > request->response.send_buffer)
				{
					debug(" Sending buffer contents.\n");
					send_buffer(connection);
				}
				/* Here we simply wait for the TCP sent callback to
				 * change the state, when the last data have been
				 * sent. */
				debug(" Waiting for message dispatch on connection %p.\n", connection);
				
				if (request->response.send_buffer_pos == request->response.send_buffer)
				{
					//Done sending, print log line.
					http_print_clf_status(request); 
				}
				break;
			case HTTP_STATE_DONE:
				debug("Closing connection %p.\n", connection);
				tcp_disconnect(connection);

				if (request->response.handlers)
				{
					request->response.handlers->destroy_cb(request);
				}
				http_free_request(request);
				connection->user = NULL;
					
				//Remove from response buffer.
				debug(" Response buffer pointer %p.\n", connection_ptr);
				if (connection_ptr)
				{
					debug(" Freeing response buffer pointer.\n");
					db_free(connection_ptr);
					connection_ptr = NULL;
					
				}
				if (request->type != HTTP_NONE)
				{
					//Requeest has been parsed, decrease mutex.
					http_response_mutex--;
				}
				//Answer buffered request.
				debug(" Response handler mutex %d.\n", http_response_mutex);
				debug(" %d buffered Requests.\n", request_buffer.count); 
				if ((!http_response_mutex) && (request_buffer.count > 0))
				{
					debug(" Handling request from buffer.\n");
					connection_ptr = ring_pop_front(&request_buffer);
					if (connection_ptr)
					{
						http_response_mutex++;
						http_process_response(*((struct tcp_connection **)connection_ptr));
					}
				}
				break;
			case HTTP_STATE_ERROR:
				warn("HTTP response status: %d.\n", request->response.status_code);
				fall_through_status_handler(connection, request->response.status_code);
				send_buffer(connection);
				request->response.state = HTTP_STATE_ASSEMBLED;
				break;
			default: 
				warn("Unknown HTTP response state %d.\n", request->response.state);
		}
		//Decrease recursion level.
		request->response.level--;
		debug(" <- Recursion level: %d.\n", request->response.level);
	}
	else
	{
		debug("No request parsed yet.\n");
	}
}
