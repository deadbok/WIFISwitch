
/**
 * @file network.c
 * 
 * @brief Maps `/rest/net/network` to a JSON object with network info.
 * 
 * REST interface for getting/setting the current network name, and 
 * hostname, and getting the IP address.
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
#include "ip_addr.h"
#include "user_interface.h"
#include "tools/jsmn.h"
#include "tools/json-gen.h"
#include "user_config.h"
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "slighttp/http-handler.h"
#include "slighttp/http-response.h"

/**
 * @brief Create the GET response.
 * 
 * @param request Request to respond to..
 * @return Size of the response.
 */
static signed int create_get_response(struct http_request *request)
{
    debug("Creating network REST GET response.\n");

	if (!request->response.message)
	{	
		char *pair;
		char *response;
		struct station_config wifi_config;
		
		if (!wifi_station_get_config(&wifi_config))
		{
			error(" Could not get station configuration.\n");
			return(0);
		}
				
		pair = json_create_pair("network", (char *)wifi_config.ssid, true);
		response = json_add_to_object(NULL, pair);
		db_free(pair);
		
		char *hostname = NULL;
		hostname = wifi_station_get_hostname();
		if (!hostname)
		{
			error("Could not get hostname.\n");
			return(0);
		}

		pair = json_create_pair("hostname", hostname, true);
		response = json_add_to_object(response, pair);
		db_free(pair);
		
		struct ip_info ip_inf;		
		if (!wifi_get_ip_info(STATION_IF, &ip_inf))
		{
			error("Could not get IP address.\n");
			return(0);
		}

		char ip[12];
		os_sprintf(ip, IPSTR, IP2STR(&ip_inf.ip));
		pair = json_create_pair("ip_addr", ip, true);
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
 * @brief Create PUT response.
 * 
 * Save hostname and network name if send in the request.
 * 
 * @param request Request to respond to..
 * @return Size of the response.
 */
static signed int create_put_response(struct http_request *request)
{
	unsigned int i;
	
	debug("Creating network REST PUT response.\n");
	debug(" Request message: %s.\n", request->message);
	
	jsmn_parser parser;
	jsmntok_t tokens[3];
	int n_tokens;

	jsmn_init(&parser);
	n_tokens  = jsmn_parse(&parser, request->message, os_strlen(request->message), tokens, 5);
	//We expect at least 2 tokens in an object.
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
			if (strncmp(request->message + tokens[i].start, "network", 5) == 0)
			{
				debug(" JSON network name.\n");
				i++;
				if (tokens[i].type == JSMN_STRING)
				{
					debug(" JSON string comes next.\n");
					struct station_config sc;
					
					sc.bssid_set = 0;
					os_memcpy(&sc.ssid, request->message + tokens[i].start, tokens[i].end - tokens[i].start);
					sc.ssid[tokens[i].end - tokens[i].start] = '\0';
					debug(" Network name %s.\n", sc.ssid);
					if (!wifi_station_set_config(&sc))
					{
						error("Could set not network name.\n");
					}
				}
			}
			if (strncmp(request->message + tokens[i].start, "hostname", 5) == 0)
			{
				debug(" JSON host name.\n");
				i++;
				if (tokens[i].type == JSMN_STRING)
				{
					debug(" JSON string comes next.\n");
					char name[32];
					
					os_memcpy(name, request->message + tokens[i].start, tokens[i].end - tokens[i].start);
					name[tokens[i].end - tokens[i].start] = '\0';
					debug(" Hostname %s.\n", name);
					if (!wifi_station_set_hostname(name))
					{
						error("Could not set hostname.\n");
					}
				}
			}
		}
	}

	return(0);
}

/**
 * @brief REST handler to get/set the default network.
 *
 * @param request The request that we're handling.
 * @return Bytes send.
 */
signed int http_rest_network_handler(struct http_request *request)
{
	if (!request)
	{
		warn("Empty request.\n");
		return(RESPONSE_DONE_ERROR);
	}
	if (os_strncmp(request->uri, "/rest/net/network\0", 18) != 0)
    {
		debug("Rest handler network will not handle request.\n"); 
		return(RESPONSE_DONE_CONTINUE);
    }
	
	return(http_simple_GET_PUT_handler(request, create_get_response, create_put_response, NULL));
}
