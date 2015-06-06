/**
 * @file net-names.c
 *
 * @brief REST interface for network names.
 * 
 * Maps `/rest/net/networks/` to a list of all access points.
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
#include "tools/missing_dec.h"
#include "tools/strxtra.h"
#include "slighttp/http.h"
#include "fs/fs.h"

/**
 * @brief Tell if we will handle a certain URI.
 * 
 * @param uri The URI to test.
 * @return True if we can handle the URI.
 */
bool ICACHE_FLASH_ATTR rest_net_names_test(char *uri)
{
    if (os_strncmp(uri, "/rest/net/networks/", 19) == 0)
    {
        return(true);
    }
    return(false);  
}

/**
 * @brief Callback for when the ESP8266 is done fincing access points.
 * 
 * @param arg Pointer to a ESP8266 scaninfo struct, with an AP list.
 * @param status ESP8266 enum STATUS, telling how the scan went.
 */
static void scan_done_cb(void *arg, STATUS status)
{
	scaninfo *info = arg; 
	struct bss_info *ap_info; 
	
	debug("AP scan callback for REST.\n");

	if (status == OK)
	{
		ap_info = (struct bss_info *)arg;
		//Skip first according to docs.
		ap_info = ap_info->next.stqe_next; 
		
		while (ap_info)
		{
			debug(" BSSID: %s.\n", ap_info->ssid);
			ap_info = ap_info->next.stqe_next;
		}
			
	}	
	else
	{
		error(" Scanning AP's.\n");
	}
}

/**
 * @brief Generate the HTML for configuring the network connection.
 * 
 * @param uri The URI to answer.
 * @param request Data for the request that got us here.
 * @return The HTML.
 */
char ICACHE_FLASH_ATTR *rest_net_names_html(char *uri, struct http_request *request)
{
    char *ret = NULL;
    
    debug("In network names REST handler (%s).\n", uri);
 
	if (request->type == HTTP_GET)
	{
		if (wifi_station_scan(NULL, &scan_done_cb))
		{
			debug(" Scanning for AP's.\n");
		}
		else
		{
			error(" Could not scan AP's.\n");
		}
	}
    return(ret);
}

/**
 * @brief Deallocate memory used for the HTML.
 * 
 * @param html A pointer to the mem to deallocate.
 */
void ICACHE_FLASH_ATTR rest_net_names_destroy(char *html)
{
	debug("Freeing static network config data.\n");
}
