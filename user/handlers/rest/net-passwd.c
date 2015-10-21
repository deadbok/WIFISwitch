
/**
 * @file net-passwd.c
 *
 * @brief REST interface for setting the current network password.
 * 
 * Maps `/rest/net/password` to the password of the configures access point, but
 * only PUT method is supported.
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
#include "c_types.h"
#include "eagle_soc.h"
#include "osapi.h"
#include "user_interface.h"
#include "tools/jsmn.h"
#include "user_config.h"
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "slighttp/http-response.h"
#include "slighttp/http-handler.h"


/**
 * @brief Keep track of actual password changes.
 */
static bool passwd_changed = false;

/**
 * @brief Respond to a PUT request.
 *
 * Set the password used to connect to the configured WIFI.
 * 
 * @param request The request that we're handling.
 * @return Bytes send.
 */
static signed int create_put_response(struct http_request *request)
{
	unsigned int i;
	jsmn_parser parser;
	jsmntok_t tokens[3];
	int n_tokens;

	jsmn_init(&parser);
	n_tokens  = jsmn_parse(&parser, request->message, os_strlen(request->message), tokens, 3);
	//We expect 3 tokens in an object.
	if ( (n_tokens < 3) || tokens[0].type != JSMN_OBJECT)
	{
		warn("Could not parse JSON request.\n");
		return(RESPONSE_DONE_ERROR);				
	}
	for (i = 1; i < n_tokens; i++)
	{
		debug(" JSON token %d.\n", i);
		if (tokens[i].type == JSMN_STRING)
		{
			debug(" JSON token start with a string.\n");
			if (strncmp(request->message + tokens[i].start, "password", 5) == 0)
			{
				debug(" JSON password.\n");
				i++;
				if (tokens[i].type == JSMN_STRING)
				{
					debug(" JSON string comes next.\n");
					struct station_config sc;
					
					wifi_station_get_config(&sc);
					sc.bssid_set = 0;
					os_memcpy(&sc.password, request->message + tokens[i].start, tokens[i].end - tokens[i].start);
					sc.password[tokens[i].end - tokens[i].start] = '\0';
					debug(" Network password %s.\n", sc.password);
					if (!wifi_station_set_config(&sc))
					{
						error("Could not network name.\n");
					}
					passwd_changed = true;
				}
			}
		}
	}
	request->response.message_size = 0;
	return(0);
}

/**
 * @brief REST handler for setting the password for the default network.
 *
 * @param request The request that we're handling.
 * @return Bytes send.
 */
signed int http_rest_net_passwd_handler(struct http_request *request)
{
	if (!request)
	{
		warn("Empty request.\n");
		return(RESPONSE_DONE_ERROR);
	}
	if (request->type != HTTP_PUT)
	{
		debug("REST handler network password only supports PUT.\n");
		return(RESPONSE_DONE_CONTINUE);
	}

    if (os_strncmp(request->uri, "/rest/net/password\0", 19) != 0)
    {
		debug("REST handler network password will not handle request,\n"); 
        return(RESPONSE_DONE_CONTINUE);
    }
	
	return(http_simple_GET_PUT_handler(request, NULL, create_put_response, NULL));
}
