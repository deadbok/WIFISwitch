
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
#include "slighttp/http-handler.h"

/**
 * @brief AP names.
 */
static char **ap_ssids;
/**
 * @brief number of AP names.
 */
static unsigned short n_aps;
/**
 * @brief Stores the request while waiting for the callback.
 */
static struct http_request *waiting_request = NULL;
/**
 * @brief Statuf of SDK call.
 */
static bool error = false;

/**
 * @brief Context for the network name handler.
 */
struct rest_net_names_context
{
	/**
	 * @brief The response.
	 */
	char *response;
	/**
	 * @brief The number of bytes to send.
	 */
	size_t size;
};

/**
 * @brief Create a JSON array of strings.
 * 
 * @param values Array of strings.
 * @param entries Number of strings in array.
 * @return The JSON array.
 */
static char ICACHE_FLASH_ATTR *json_create_string_array(char **values, size_t entries)
{
	size_t i;
	size_t total_length = 0;
	size_t *lengths = db_malloc(sizeof(size_t) * entries, "lengths json_create_string_array");
	char *ret, *ptr;
	
	//Get the length of the JSON array.
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
	
	//Build the actual JSON array as a string.
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
 * @brief Callback for when the ESP8266 is done finding access points.
 * 
 * This sends off the headers, restarting the send cycle.
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
	struct http_request *request;
	
	debug("AP scan callback for REST.\n");
	if (status == OK)
	{
		debug(" Scanning went OK (%p).\n", arg);
		scn_info = (struct bss_info *)arg;
		debug(" Processing AP list (%p, %p).\n", scn_info, scn_info->next.stqe_next);
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
		error = true;
	}
	request = waiting_request;
	if (request->connection)
	{
		if (!error)
		{
			((struct rest_net_names_context *)request->response.context)->response = json_create_string_array(ap_ssids, n_aps);
			((struct rest_net_names_context *)request->response.context)->size = os_strlen(((struct rest_net_names_context *)request->response.context)->response);
		}
		else
		{
			((struct rest_net_names_context *)request->response.context)->response = "[\"error\"]";
			((struct rest_net_names_context *)request->response.context)->size = os_strlen(((struct rest_net_names_context *)request->response.context)->response);
		}
		if (n_aps && ap_ssids)
		{
			//Free old data if there is any.
			for (i = 0; i < n_aps; i++)
			{
				db_free(ap_ssids[i]);
			}
			db_free(ap_ssids);
			ap_ssids = NULL;
			n_aps = 0;
		}

		//We'll be sending headers next.
		request->response.state = HTTP_STATE_HEADERS;
		http_handle_response(request);
	}
	else
	{
		error(" No connection for sending network names.\n");
	}
}

/**
 * @brief Start scanning for WIFI network names.
 * 
 * @param request The request that brought us here.
 */
static void scan_net_names(struct http_request *request)
{
	debug("Start network names scan.\n");

	//Get set if there is an error in the callback.
	error = false;
	//Prevent more scans until the call back has done it job.
	waiting_request = request;
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

/**
 * @brief REST handler to get/set the GPIO state and get GPIO information.
 *
 * @param request The request that we're handling.
 * @return Bytes send.
 */
signed int ICACHE_FLASH_ATTR http_rest_net_names_handler(struct http_request *request)
{
	signed int ret = 0;
		
	if (!request)
	{
		warn("Empty request.\n");
		return(RESPONSE_DONE_ERROR);
	}
	if ((request->type != HTTP_GET) &&
		(request->type != HTTP_HEAD))
	{
		debug(" Rest handler net-names only supports HEAD, GET.\n");
		return(RESPONSE_DONE_CONTINUE);
	}
	if (os_strncmp(request->uri, "/rest/net/networks\0", 19) != 0)
    {
		debug("Rest handler net-names will not handle request,\n");
        return(RESPONSE_DONE_CONTINUE);
    }
     
	if (request->response.state == HTTP_STATE_NONE)
	{
		if ((request->type == HTTP_HEAD) ||
		    (request->type == HTTP_GET))
		{
			if (!waiting_request)
			{
				//Only do stuff if we're not already waiting. 
				//Get some mem if we need it.
				if (!request->response.context)
				{
					request->response.context = db_zalloc(sizeof(struct rest_net_names_context), "request->response.context scan_done_cb");
				}
				//Start scanning.
				if (!((struct rest_net_names_context *)request->response.context)->response)
				{
					scan_net_names(request);
				}
			}
			//Done, scan callback will take over and clean up afterwards.
			return(RESPONSE_DONE_NO_DEALLOC);
		}
	}
	
	if (request->response.state == HTTP_STATE_HEADERS)
	{
		if ((request->type == HTTP_HEAD) ||
		    (request->type == HTTP_GET))
		{
			//This is the answer, we're called from the scan callback.
			request->response.status_code = 200;
			//Send status and headers.
			ret += http_send_status_line(request->connection, request->response.status_code);
			ret += http_send_default_headers(request, ((struct rest_net_names_context *)request->response.context)->size, "json");
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
	}
	
	if (request->response.state == HTTP_STATE_MESSAGE)
	{
		if (request->type == HTTP_GET)
		{
			debug(" Response: %s.\n", ((struct rest_net_names_context *)request->response.context)->response);
			ret += http_send(request->connection, 
					 ((struct rest_net_names_context *)request->response.context)->response,
					 ((struct rest_net_names_context *)request->response.context)->size);
			request->response.state = HTTP_STATE_DONE;
			request->response.message_size = ((struct rest_net_names_context *)request->response.context)->size;
			return(ret);
		}
	}
	    
	if (request->response.state == HTTP_STATE_DONE)
	{
		debug("Freeing network names REST handler data (%p).\n", request->response.context);	
		if (request->response.context)
		{	
			if (((struct rest_net_names_context *)request->response.context)->response)
			{
				db_free(((struct rest_net_names_context *)request->response.context)->response);
			}
			db_free(request->response.context);
			request->response.context = NULL;
			waiting_request = NULL;
		}
	}
	return(RESPONSE_DONE_FINAL);
}
