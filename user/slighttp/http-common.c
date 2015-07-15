/** @file http.c
 *
 * @brief Common functions for the HTTP server.
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
#include <stdlib.h>
#include "tools/missing_dec.h"
#include "c_types.h"
#include "osapi.h"
#include "mem.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_config.h"
#include "net/tcp.h"
#include "http.h"
#include "http-common.h"

/**
 * @brief Test if a header is set to a certain value.
 * 
 * @param request HTTP request to check.
 * @param name Name of the header. *Must be lower case.*
 * @param value Value to test for. *Case sensitive.*
 * @return `true`if found, `false` otherwise.
 */
bool ICACHE_FLASH_ATTR is_header_value(struct http_request *request, char *name, char *value)
{
    unsigned short i;
    char *current_name;
    char *current_value;
    
    for (i = 0; i < request->n_headers; i++)
    {
        current_name = request->headers[i].name;
        current_value = request->headers[i].value;
        if ((os_strcmp(current_name, name) == 0) && 
            (os_strcmp(current_value, value) == 0))
        {
            return(true);
        }
    }
    return(false);
}

/**
 * @brief Get the value of a header by name.
 * 
 * @param request HTTP request to look for the header in.
 * @param name Name of the header.
 * @return NULL if not found. The value if found.
 */
char ICACHE_FLASH_ATTR *get_header_value(struct http_request *request, char *name)
{
	unsigned short i;
	
	for (i = 0; i < request->n_headers; i++)
	{
        if (os_strcmp(request->headers[i].name, name) == 0)
        {
            return(request->headers[i].value);
        }
    }
    return(NULL);
}		

/**
 * @brief Print a Common Log Format message to the console.
 * 
 * Log access to the web server to console in a standardized text file format:
 * 
 *  host ident authuser date request status bytes
 * 
 * or:
 * 
 *  127.0.0.1 user-identifier frank [10/Oct/2000:13:55:36 -0700] "GET /apache_pb.gif HTTP/1.0" 200 2326
 * 
 * See [CLF](https://en.wikipedia.org/wiki/Common_Log_Format).
 */
void ICACHE_FLASH_ATTR http_print_clf_status(struct http_request *request)
{
    char *unknown = "-";
    
	db_printf(IPSTR, IP2STR(request->connection->conn->proto.tcp->remote_ip));
	db_printf(" %s %s %s", unknown, unknown, unknown);
	db_printf(" \"");
	switch(request->type)
	{
		case HTTP_OPTIONS: db_printf("OPTIONS");
						   break;
		case HTTP_GET: db_printf("GET");
					   break;
    	case HTTP_HEAD: db_printf("HEAD");
						break;
    	case HTTP_POST: db_printf("POST");
						break;
    	case HTTP_PUT: db_printf("PUT");
					   break;
    	case HTTP_DELETE: db_printf("DELETE");
						  break;
    	case HTTP_TRACE: db_printf("TRACE");
						 break;
    	case HTTP_CONNECT: db_printf("CONNECT");
						   break;
    	default: db_printf("-");
    }
    db_printf(" HTTP/%s %d %d\n", request->version, request->response.status_code, request->response.message_size);
}

