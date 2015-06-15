/** @file http.c
 *
 * @brief Simple HTTP server for the ESP8266.
 * 
 * Simple HTTP server, that only supports the most basic functionality, and is
 * small.
 * 
 * To keep memory use small, the data passed to the server, from the network, is 
 * expected to stay until the server is done with them. Instead of making a copy
 * of the data from the network, the server inserts `\0` characters directly into
 * the data. The server then point to the values directly in the network data.
 * 
 * What works:
 * - GET requests.
 * - HEAD requests.
 * - POST requests.
 * - CRLF, LF, and space tolerance (never tested except for space).
 * - File system access.
 * 
 * The server can have different document roots for pages loaded from the file
 * system, but the error pages like `404.html` are always loaded from the root.
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
#include "http-common.h"
#include "http-tcp.h"
#include "http.h"

/**
 * @brief Where to serve files from, in the file system.
 */
char *http_fs_doc_root;
/**
 * @brief Initialise the HTTP server,
 * 
 * @param path Path to search as root of the server in the file system.
 * @param builtin_uris An array of built in handlers for URIs.
 * @param n_builtin_uris Number of URI handlers.
 * @return `true`on success.
 */
bool ICACHE_FLASH_ATTR init_http(char *path, struct http_builtin_uri *builtin_uris, unsigned short n_builtin_uris)
{
    n_static_uris = n_builtin_uris;
    static_uris = builtin_uris;
    
    http_fs_doc_root = path;
    
    //Initialise TCP and listen on port 80.
    if (!init_tcp())
    {
		return(false);
	}
	if (tcp_listen(80, tcp_connect_cb, tcp_reconnect_cb, tcp_disconnect_cb, 
				   tcp_write_finish_cb, tcp_recv_cb, tcp_sent_cb))
	{
		return(false);
	}
	return(true);
}
