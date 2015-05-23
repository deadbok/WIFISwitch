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
#include "mem.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_config.h"
#include "net/tcp.h"
#include "tools/strxtra.h"
#include "fs/fs.h"
#include "http-common.h"
#include "http.h"

/**
 * @brief Send a HTTP response.
 * 
 * Send a HTTP response, and close the connection, if the headers says so.
 * 
 * @param connection Connection to use when sending.
 * @param response Response header (without the final `\r\n\r\n`.
 * @param content The content of the response.
 * @param send_content If `false` only send headers.
 * @param close Close the connection after the response. 
 */
void ICACHE_FLASH_ATTR send_response(struct tcp_connection *connection,
                                     char *response,
                                     char *content,
                                     bool send_content,
                                     bool close)
{
    char *h_connection_close = "Connection: close\r\n";
    char *h_length = "Content-Length: ";
    char status_code[4] = "000\0";
    char html_length[10];
    char *s_status_code;
 
    //Send start of the response.
    tcp_send(connection, response);
    //Send close announcement if needed.
    if (close)
    {
        tcp_send(connection, h_connection_close);
    }

    //Send the length header.
    tcp_send(connection, h_length);
    //Get the message length.
    os_sprintf(html_length, "%d", os_strlen(content));
    //Send it.
    tcp_send(connection, html_length);
    //End of header.
    tcp_send(connection, "\r\n\r\n");
    //Send content.
    if (send_content)
    {
        tcp_send(connection, content);
    }
    //Find status code and skip spaces.
    for (s_status_code = os_strstr(response, " "); *s_status_code == ' '; s_status_code++);
    if (s_status_code)
    {
        os_memcpy(status_code, s_status_code, 3);
        print_clf_status(connection, status_code, html_length);
    }
    else
    {
        print_clf_status(connection, "-", html_length);
    }
    if (close)
    {
        tcp_disconnect(connection);
    }
}

