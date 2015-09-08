
/**
 * @file network.c
 *
 * @brief REST interface for getting the current network name.
 * 
 * Maps `/rest/net/network` to the name of the configures access point.
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
#include "json/jsonparse.h"
#include "user_config.h"
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "slighttp/http-response.h"
#include "slighttp/http-handler.h"

/**
 * @brief Create the response.
 * 
 * @param request Request to respond to..
 * @return Size of the response.
 */
static signed int create_response(struct http_request *request)
{
	struct station_config wifi_config;
	char *response;
	char *response_pos;
	    
    debug("Creating network REST response.\n");

	if (!request->response.context)
	{	
		response = db_malloc(sizeof(char) * 51, "response create_response");
		response_pos = response;
		
		os_strcpy(response, "{ \"network\" : \"");
		response_pos += 15;

		if (!wifi_station_get_config(&wifi_config))
		{
			error(" Could not get station configuration.\n");
			return(0);
		}
		os_strcpy(response_pos, (char *)wifi_config.ssid);
		response_pos += os_strlen((char *)wifi_config.ssid);
		
		os_strcpy(response_pos, "\" }");
		response_pos += 3;
		*response_pos = '\0';
	
		request->response.context = response;
	}
	return(os_strlen(request->response.context));
}

/**
 * @brief REST handler to get/set the default network.
 *
 * @param request The request that we're handling.
 * @return Bytes send.
 */
signed int ICACHE_FLASH_ATTR http_rest_network_handler(struct http_request *request)
{
	int type;
	char net_name[32];
	signed int ret = 0;
	size_t msg_size = 0;
	struct station_config sc;
	struct jsonparse_state json_state;
		
	if (!request)
	{
		warn("Empty request.\n");
		return(RESPONSE_DONE_ERROR);
	}
	if ((request->type != HTTP_GET) &&
		(request->type != HTTP_HEAD) &&
		(request->type != HTTP_PUT))
	{
		debug(" File system handler only supports HEAD, GET, and PUT.\n");
		return(RESPONSE_DONE_CONTINUE);
	}
	if (os_strncmp(request->uri, "/rest/net/network\0", 18) != 0)
    {
		debug("Rest handler network will not handle request.\n"); 
		return(RESPONSE_DONE_CONTINUE);
    }
	
	if (request->response.state == HTTP_STATE_NONE)
	{
		if ((request->type == HTTP_HEAD) ||
		    (request->type == HTTP_GET))
		{
			msg_size = create_response(request);
			//We have not send anything.
			request->response.message_size = 0;
			//Send status and headers.
			ret += http_send_status_line(request->connection, request->response.status_code);
			ret += http_send_default_headers(request, msg_size, "json");
			if (request->type == HTTP_HEAD)
			{
				request->response.state = HTTP_STATE_DONE;
				return(ret);
			}
			else
			{
				//Go on to sending the file.
				request->response.state = HTTP_STATE_MESSAGE;
			}
		}
		else
		{
			//No data.
			request->response.status_code = 204;
			//We have not send anything.
			request->response.message_size = 0;
			//Send status and headers.
			ret += http_send_status_line(request->connection, request->response.status_code);
			ret += http_send_default_headers(request, 0, NULL);
			//Go on to set the network.
			request->response.state = HTTP_STATE_MESSAGE;
		}
	}
	
	if (request->response.state == HTTP_STATE_MESSAGE)
	{
		if (request->type == HTTP_GET)
		{
			msg_size = os_strlen(request->response.context);
			debug(" Response: %s.\n", (char *)request->response.context);
			ret += http_send(request->connection, request->response.context, msg_size);
			//We're done sending the message.
			request->response.state = HTTP_STATE_DONE;
			request->response.message_size = msg_size;
			return(ret);
		}
		else
		{
			debug(" Network selected: %s.\n", request->message);
			jsonparse_setup(&json_state, request->message, os_strlen(request->message));
			while ((type = jsonparse_next(&json_state)) != 0)
			{
				if (type == JSON_TYPE_PAIR_NAME)
				{
					if (jsonparse_strcmp_value(&json_state, "network") == 0)
					{
						debug(" Found network value in JSON.\n");
						jsonparse_next(&json_state);
						jsonparse_next(&json_state);
						jsonparse_copy_value(&json_state, net_name, sizeof(net_name));
						debug(" Name: %s.\n", net_name);

						sc.bssid_set = 0;
						os_memcpy(&sc.ssid, net_name, 32);
						sc.password[0] = '\0';
						wifi_station_set_config(&sc);
					}
				}
			}
			request->response.message_size = 0;
			return(ret);
		}
	}
	    
	if (request->response.state == HTTP_STATE_DONE)
	{
		if ((request->type == HTTP_HEAD) ||
		    (request->type == HTTP_GET))
		{
			debug("Freeing network name REST handler data.\n");
			if (request->response.context)
			{
				db_free(request->response.context);
				request->response.context = NULL;
			}
		}
	}
	return(RESPONSE_DONE_FINAL);
}	
	
	
