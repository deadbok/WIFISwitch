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
#include "user_config.h"
#include "tools/missing_dec.h"
#include "tools/strxtra.h"
#include "slighttp/http.h"
#include "fs/fs.h"

static char **ap_ssids;
static unsigned short n_aps;
static bool updating;

static char ICACHE_FLASH_ATTR *json_create_string_array(char **values, size_t entries)
{
	size_t i;
	size_t total_length = 0;
	size_t *lengths = db_malloc(sizeof(size_t) * entries, "lengths json_create_string_array");
	char *ret, *ptr;
	
	//'['
	total_length++;
	for (i = 0; i < entries; i++)
	{
		//'\"'
		total_length++;
		lengths[i] = os_strlen(values[i]);
		total_length += lengths[i];
		//"\","
		total_length++;
		if (i != (entries -1))
		{
			total_length++;
		}

	}
	//']'
	total_length++;
	
	ret = (char *)db_malloc(total_length, "res json_create_string_array");
	ptr = ret;
	
	*ptr++ = '[';
	for (i = 0; i < entries; i++)
	{
		*ptr++ = '\"';
		os_memcpy(ptr, values[i], lengths[i]);
		ptr += lengths[i];
		*ptr++ = '\"';
		if (i != (entries -1))
		{
			*ptr++ = ',';
		}
	}
	*ptr++ = ']';
	*ptr = '\0';
	
	db_free(lengths);
	
	return(ret);
}

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
	struct bss_info *current_ap_info;
	struct bss_info *scn_info;
	unsigned short i;
	size_t size;	
	
	updating = true;
	
	debug("AP scan callback for REST.\n");
	if (status == OK)
	{
		debug(" Scanning went OK (%p).\n", arg);
		scn_info = (struct bss_info *)arg;
		debug(" Processing AP list (%p, %p).\n", scn_info, scn_info->next.stqe_next);
		//Free old data if there is any.
		if (ap_ssids)
		{
			for (i = 0; i < n_aps; i++)
			{
				db_free(ap_ssids[i]);
			}
			db_free(ap_ssids);
			n_aps = 0;
		}
		debug(" Counting AP's.");
		//Skip first according to docs.
		for (current_ap_info = scn_info->next.stqe_next;
			 current_ap_info != NULL;
			 current_ap_info = current_ap_info->next.stqe_next)
		{
			debug(".");
			n_aps++;
		}
		debug("%d.\n", n_aps);
		if (n_aps)
		{

			//Get mem for array.
			ap_ssids = db_zalloc(sizeof(char *) * n_aps, "ap_ssids rest_net_names_html");
			//Fill in the AP names.
			debug(" AP names:\n");
			current_ap_info = scn_info->next.stqe_next;
			for (i = 0; i < n_aps; i++)
			{
				size = sizeof(char) * (strlen((char *)current_ap_info->ssid));
				ap_ssids[i] = db_malloc(size + 1, "ap_ssids[] rest_net_names_html");
				os_memcpy(ap_ssids[i], current_ap_info->ssid, size);
				ap_ssids[i][size] = '\0';
				current_ap_info = current_ap_info->next.stqe_next;
				debug("  %s.\n", ap_ssids[i]);
			}
		}
	}	
	else
	{
		error(" Scanning AP's.\n");
	}
	updating = false;
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
	char *scan_str[1]={"scanning..."};
	    
    debug("In network names REST handler (%s).\n", uri);

	if (request->type == HTTP_GET)
	{
		if (!updating)
		{
			if (n_aps && ap_ssids)
			{
				ret = json_create_string_array(ap_ssids, n_aps);
			}

			debug(" Starting scan.\n");
			if (wifi_station_scan(NULL, &scan_done_cb))
			{
				debug(" Scanning for AP's.\n");
			}
			else
			{
				error(" Could not scan AP's.\n");
			}				
		}
	}
	
	if (!ret)
	{
		ret = json_create_string_array(scan_str, 1);
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
	unsigned short i;
	
	debug("Freeing network names REST data.\n");
	if (html)
	{
		debug(" Freeing HTML (%p).\n", html);
		db_free(html);
	}
	/*if (ap_ssids)
	{
		for (i = 0; i < n_aps; i++)
		{
			debug(" Freeing AP name %d (%p).\n", i, ap_ssids[i]);
			db_free(ap_ssids[i]);
		}
		debug(" Freeing AP names array (%p).\n", ap_ssids);
		db_free(ap_ssids);
		n_aps = 0;
	}*/
}
