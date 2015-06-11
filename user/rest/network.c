
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
#include "tools/missing_dec.h"
#include "tools/strxtra.h"
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "fs/fs.h"

/**
 * @brief Return string with JSON.
 */
static char ret[49];

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
char ICACHE_FLASH_ATTR *rest_network(char *uri, struct http_request *request, struct http_response *response)
{
	struct station_config wifi_config;
	char *ret_pos = ret;
	    
    debug("In network name REST handler (%s).\n", uri);
	
	if (request->type == HTTP_GET)
	{
		os_strcpy(ret, "{ network : \"");
		ret_pos += 13;

		if (!wifi_station_get_config(&wifi_config))
		{
			error(" Could not get station configuration.\n");
			return(NULL);
		}
		os_strcpy(ret_pos, (char *)wifi_config.ssid);
		ret_pos += os_strlen((char *)wifi_config.ssid);
		
		os_strcpy(ret_pos, "\" }");
		ret_pos += 3;
		*ret_pos = '\0';
	}
	
	debug(" Response: %s.\n", ret);
    return(ret);
}

/**
 * @brief Deallocate memory used for the HTML.
 * 
 * @param html A pointer to the mem to deallocate.
 */
void ICACHE_FLASH_ATTR rest_network_destroy(char *response)
{
}
