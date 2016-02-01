
/**
 * @file version.c
 *
 * @brief REST interface for getting the firmware version.
 * 
 * Maps `/rest/fw/version` to a JSON object of versions.
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
#include "tools/json-gen.h"
#include "fs/dbffs.h"
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
    debug("Creating version REST response.\n");

	if (!request->response.message)
	{	
		char *pair;		
		char *response;
		
		pair = json_create_pair("fw_ver", VERSION, true);
		response = json_add_to_object(NULL, pair);
		db_free(pair);
	
		pair = json_create_pair("httpd_ver", HTTP_SERVER_VERSION, true);
		response = json_add_to_object(response, pair);
		db_free(pair);

		pair = json_create_pair("dbffs_ver", DBFFS_VERSION, true);
		response = json_add_to_object(response, pair);
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
 * @brief REST handler to get/set the default network.
 *
 * @param request The request that we're handling.
 * @return Bytes send.
 */
signed int http_rest_version_handler(struct http_request *request)
{
	if (!request)
	{
		warn("Empty request.\n");
		return(RESPONSE_DONE_ERROR);
	}
	if (os_strncmp(request->uri, "/rest/fw/version\0", 17) != 0)
    {
		debug("REST version handler will not handle request.\n"); 
		return(RESPONSE_DONE_CONTINUE);
    }
	
	return(http_simple_GET_PUT_handler(request, create_get_response, NULL, NULL));
}
