
/**
 * @file mem.c
 *
 * @brief REST interface for getting memory info.
 * 
 * Maps `/rest/fw/mem` to a JSON object with memory info.
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
#include "osapi.h"
#include "c_types.h"
#include "user_interface.h"
#include "user_config.h"
#include "tools/strxtra.h"
#include "tools/json-gen.h"
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "slighttp/http-handler.h"
#include "slighttp/http-response.h"
#include "debug.h"

/**
 * @brief Create the response.
 * 
 * @param request Request to respond to..
 * @return Size of the response.
 */
static signed int create_get_response(struct http_request *request)
{  
    debug("Creating memory REST response.\n");

	if (!request->response.message)
	{	
		char *pair;
		char *response;
		char free_mem[10];

		if(!itoa(system_get_free_heap_size(), free_mem, 10))
		{
			error("Could not get free memory size.\n");
			return(0);
		}
		pair = json_create_pair("free", free_mem, true);
		response = json_add_to_object(NULL, pair);
		db_free(pair);
	
		request->response.message = response;
	}
	else
	{
		warn("Message is already set.\n");
	}
	return(os_strlen(request->response.message));
}

/**
 * @brief REST handler to get free mem.
 *
 * @param request The request that we're handling.
 * @return Bytes send.
 */
signed int http_rest_mem_handler(struct http_request *request)
{
	if (!request)
	{
		warn("Empty request.\n");
		return(RESPONSE_DONE_ERROR);
	}
	if (os_strncmp(request->uri, "/rest/fw/mem\0", 13) != 0)
    {
		debug("REST memory handler will not handle request.\n"); 
		return(RESPONSE_DONE_CONTINUE);
    }
	
	return(http_simple_GET_PUT_handler(request, create_get_response, NULL, NULL));
}
