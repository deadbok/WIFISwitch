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
 * 
 * **This server is not compliant with HTTP, to save space, stuff has been
 * omitted. It has been built to be small, not fast, and probably breaks in 100
 * places.**
 * 
 * I have gone to somewhat irresponsible length, to make this code dynamic
 * with regards to the size of the buffers for recieved data. This is probably
 * stupid and wastefull since, there isn't that much RAM to begin with, and the
 * failure modes, seems to become way more complex. 
 * 
 * Missing functionality:
 * - Does not understand any header fields.
 * - MIME types.
 * - File system access.
 * - 400 errors are not send in all situations where they should be.
 * - Persistent connections. The server closes every connection when the 
 *   response has been sent.
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
 * @brief Initialise the HTTP server,
 * 
 * @param builtin_uris An array of built in handlers for URIs.
 * @param n_builtin_uris Number of URI handlers.
 */
void ICACHE_FLASH_ATTR init_http(struct http_builtin_uri *builtin_uris, unsigned short n_builtin_uris)
{
    n_static_uris = n_builtin_uris;
    static_uris = builtin_uris;
    
    //Initialise TCP and listen on port 80.
    init_tcp();
    tcp_listen(80, tcp_connect_cb, tcp_reconnect_cb, tcp_disconnect_cb, 
             tcp_write_finish_cb, tcp_recv_cb, tcp_sent_cb);
}
