/** @file http.c
 *
 * @brief Simple HTTP server for the ESP8266.
 * 
 * Simple HTTP server, that only supports the most basic functionality, and is
 * small.
 * 
 * What works:
 * - GET requests.
 * - HEAD requests.
 * - POST requests.
 * - CRLF, LF, and space tolerance (never tested except for space).
 * - File system access.
 * 
 * The server can have different document roots for pages loaded from the file
 * system, including error pages like `404.html that are tried if an error
 * occurs.
 * 
 * **This server is not compliant with HTTP, to save space, stuff has been
 * omitted. It has been built to be small, not fast, and probably breaks in 100
 * places.** 
 * 
 * Missing functionality:
 * - Does not understand any header fields.
 * - 400 errors are not send in all situations where they should be.
 * - Persistent connections. The server closes every connection when the 
 *   response has been sent.
 * - Chunked transfers.
 * 
 * Things that needs to be dealt with, from the specs:
 * - Space between start line, and header (RFC7230.txt Line 1095).
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
#include "slighttp/http-common.h"
#include "slighttp/http-tcp.h"
#include "slighttp/http-handler.h"
#include "slighttp/http.h"
#include "tools/ring.h"

/**
 * @brief Server status.
 * 
 * Server status: true when listening, and false when stopped.
 */
static bool status = false;

bool init_http(unsigned int port)
{
	struct net_connection *connection;
	
	debug("Initialising HTTP server on port %d.\n", port);
    //Initialise TCP and listen on port 80.
    if (!init_tcp())
    {
		return(false);
	}
	connection = tcp_listen(port, http_tcp_connect_cb,
							http_tcp_disconnect_cb,
							http_tcp_write_finish_cb, http_tcp_recv_cb,
							http_tcp_sent_cb);
	if (!connection)
	{
		return(false);
	}
	connection->type = NET_CT_HTTP;
	//Create buffer for requests.
	debug("Creating request buffer.\n");
	init_ring(&request_buffer, sizeof(struct tcp_connection *), HTTP_REQUEST_BUFFER_SIZE);
	
	status = true;
	return(true);
}

bool http_get_status(void)
{
	return(status);
}
