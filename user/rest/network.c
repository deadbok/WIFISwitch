
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
#include "user_config.h"
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "slighttp/http-response.h"

/**
 * @brief Tell if we will handle a certain URI.
 * 
 * @param uri The URI to test.
 * @return True if we can handle the URI.
 */
bool ICACHE_FLASH_ATTR rest_network_test(char *uri)
{
    if (os_strncmp(uri, "/rest/net/network\0", 18) == 0)
    {
        return(true);
    }
    return(false);  
}

/**
 * @brief Generate the HTML for configuring the network connection.
 * 
 * @param uri The URI to answer.
 * @param request Data for the request that got us here.
 * @return The HTML.
 */
size_t ICACHE_FLASH_ATTR rest_network(char *uri, struct http_request *request)
{
	struct station_config wifi_config;
	char response[49];
	char *response_pos = response;
	char buffer[16];
	size_t msg_size = 0;
	    
    debug("In network name REST handler (%s).\n", uri);
	
	if (request->type == HTTP_GET)
	{
		http_send_header(request->connection, "Content-Type", http_mime_types[MIME_JSON].type);

		os_strcpy(response, "{ network : \"");
		response_pos += 13;

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
		
		//Get the size of the message.
		msg_size = response_pos -response - 1;
		os_sprintf(buffer, "%d", msg_size);
		http_send_header(request->connection, "Content-Length", buffer);
		tcp_send(request->connection, "\r\n", 2);
		
		debug(" Response: %s.\n", response);
		tcp_send(request->connection, response, msg_size);
	}
	
	debug(" Response size: %d.\n", msg_size);
    return(msg_size);
}

/**
 * @brief Deallocate memory used for the HTML.
 * 
 * @param html A pointer to the mem to deallocate.
 */
void ICACHE_FLASH_ATTR rest_network_destroy(void)
{
}
