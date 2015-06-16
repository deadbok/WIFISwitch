
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
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "slighttp/http-response.h"

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
bool ICACHE_FLASH_ATTR rest_net_names_test(struct http_request *request)
{
    if (os_strncmp(request->uri, "/rest/net/networks", 18) == 0)
    {
		debug("Rest handler net-names found: %s.\n", request->uri);
        return(true);
    }
    debug("Rest handler net-names will not request,\n"); 
    return(false);  
}

/**
 * @brief Callback for when the ESP8266 is done finding access points.
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
 * @brief Generate a JSON array of available access points.
 * 
 * @param uri The URI to answer.
 * @param request Data for the request that got us here.
 * @return The JSON.
 */
size_t ICACHE_FLASH_ATTR rest_net_names(struct http_request *request)
{
    char *response = NULL;
	char buffer[16];
	size_t msg_size = 0;
	char *uri = request->uri;
	    
    debug("In network names REST handler (%s).\n", uri);

	if (request->type == HTTP_GET)
	{
		http_send_header(request->connection, "Content-Type", http_mime_types[MIME_JSON].type);
		
		if (!updating)
		{
			if (n_aps && ap_ssids)
			{
				response = json_create_string_array(ap_ssids, n_aps);
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
		if (!response)
		{
			response = db_malloc(sizeof(char) * 3, "ret rest_net_names_html");
			os_memcpy(response, "[]\0", sizeof(char) * 3);
		}
		//Get the size of the message.
		msg_size = os_strlen(response);
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
 * @brief Deallocate memory used for the response.
 * 
 * @param response A pointer to the mem to deallocate.
 */
void ICACHE_FLASH_ATTR rest_net_names_destroy(struct http_request *request)
{
}
