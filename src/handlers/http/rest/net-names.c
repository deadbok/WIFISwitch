
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
#include "tools/json-gen.h"
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "slighttp/http-response.h"
#include "slighttp/http-handler.h"

/**
 * @brief Stores the request while waiting for the callback.
 */
static struct http_request *waiting_request = NULL;
/**
 * @brief Status of SDK call.
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
	size_t ssid_size;
	size_t size;
	char *json_ssid;
	char *json_ssid_pos = NULL;
	char *json_response = NULL;
	struct http_request *request;
	
	debug("AP scan callback for REST.\n");
	if (status == OK)
	{
		debug(" Scanning went OK (%p).\n", arg);
		scn_info = (struct bss_info *)arg;
		debug(" Processing AP list (%p, %p).\n", scn_info, scn_info->next.stqe_next);
		debug(" AP names:\n");
		//Skip first according to docs.
		for (current_ap_info = scn_info->next.stqe_next;
			 current_ap_info != NULL;
			 current_ap_info = current_ap_info->next.stqe_next)
		{
			debug("  %s.\n", current_ap_info->ssid);
			//SSID cannot be longer than 32 char.
			ssid_size = strlen((char *)(current_ap_info->ssid));
			if (ssid_size > 32)
			{
				//" + SSID + " + \0
				size = 35;
			}
			else
			{
				//" + " + \0
				size = ssid_size + 3;
			}
			json_ssid = db_malloc(size, "scan_done_sb json_ssid");
			json_ssid_pos = json_ssid;
			*json_ssid_pos++ = '\"';
			if (ssid_size > 32)
			{
				os_memcpy(json_ssid_pos, current_ap_info->ssid, 32);
				json_ssid_pos += 32;
			}
			else
			{
				os_memcpy(json_ssid_pos, current_ap_info->ssid, ssid_size);
				json_ssid_pos += ssid_size;
			}
			os_memcpy(json_ssid_pos, "\"\0", 2);
			json_response = json_add_to_array(json_response, json_ssid);
			db_free(json_ssid);
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
			((struct rest_net_names_context *)request->response.context)->response = json_response;
			((struct rest_net_names_context *)request->response.context)->size = os_strlen(json_response);
		}
		else
		{
			((struct rest_net_names_context *)request->response.context)->response = "[\"error\"]";
			((struct rest_net_names_context *)request->response.context)->size = os_strlen(((struct rest_net_names_context *)request->response.context)->response);
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
signed int http_rest_net_names_handler(struct http_request *request)
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
