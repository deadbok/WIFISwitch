
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
#include "json/jsonparse.h"
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
static signed int ICACHE_FLASH_ATTR create_get_response(struct http_request *request)
{
	struct station_config wifi_config;
	char *response;
	char *response_pos;
	char *hostname = NULL;
	struct ip_info ip_inf;
	    
    debug("Creating network REST GET response.\n");

	if (!request->response.message)
	{	
		response = db_malloc(sizeof(char) * 124, "response create_response");
		response_pos = response;
		
		os_strcpy(response_pos, "{ \"network\" : \"");
		response_pos += 15;

		if (!wifi_station_get_config(&wifi_config))
		{
			error(" Could not get station configuration.\n");
			return(0);
		}
		os_strcpy(response_pos, (char *)wifi_config.ssid);
		response_pos += os_strlen((char *)wifi_config.ssid); //Max 32.
		
		os_strcpy(response_pos, "\", \"hostname\": \"");
		response_pos += 16;
		
		hostname = wifi_station_get_hostname();
		if (!hostname)
		{
			error("Could not get hostname.\n");
			return(0);
		}
		os_strcpy(response_pos, hostname);
		response_pos += os_strlen(hostname); //Max 32.
		
		os_strcpy(response_pos, "\", \"ip_addr\": \"");
		response_pos += 15;
		
		if (!wifi_get_ip_info(STATION_IF, &ip_inf))
		{
			error("Could not get IP address.\n");
			return(0);
		}
		os_sprintf(response_pos, IPSTR, IP2STR(&ip_inf.ip));
		response_pos += os_strlen(response_pos); //Max 11
			
		os_strcpy(response_pos, "\" }");
		response_pos += 3;
		*response_pos = '\0';
	
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
static signed int ICACHE_FLASH_ATTR create_put_response(struct http_request *request)
{
	int type;
	char name[32];
	struct station_config sc;
	struct jsonparse_state json_state;
	
	debug("Creating network REST PUT response.\n");
	debug(" Request message: %s.\n", request->message);
	
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
				jsonparse_copy_value(&json_state, name, sizeof(name));
				debug(" Name: %s.\n", name);

				sc.bssid_set = 0;
				os_memcpy(&sc.ssid, name, 32);
				sc.password[0] = '\0';
				if (!wifi_station_set_config(&sc))
				{
					error("Could not network name.\n");
				}
			}
			if (jsonparse_strcmp_value(&json_state, "hostname") == 0)
			{
				debug(" Found hostname value in JSON.\n");
				jsonparse_next(&json_state);
				jsonparse_next(&json_state);
				jsonparse_copy_value(&json_state, name, sizeof(name));
				debug(" Hostname: %s.\n", name);

				if (!wifi_station_set_hostname(name))
				{
					error("Could not set hostname.\n");
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
signed int ICACHE_FLASH_ATTR http_rest_network_handler(struct http_request *request)
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
