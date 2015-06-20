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
void ICACHE_FLASH_ATTR http_send_status_line(struct tcp_connection *connection, unsigned short status_code)
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

