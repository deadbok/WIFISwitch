
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
static struct http_request *waiting_request = NULL;
static bool error = false;

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
    if (os_strncmp(request->uri, "/rest/net/networks\0", 19) == 0)
    {
		debug("Rest handler net-names found: %s.\n", request->uri);
        return(true);
    }
    debug("Rest handler net-names will not handle request,\n"); 
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
		error = true;
	}
	request = waiting_request;
	waiting_request = NULL;
	http_process_response(request->connection);
}

static void create_response(struct http_request *request)
{
	unsigned short i;
	
	if (!waiting_request)
	{
		debug("Creating network names REST response.\n");
		if (n_aps && ap_ssids)
		{
			((struct rest_net_names_context *)request->response.context)->response = json_create_string_array(ap_ssids, n_aps);
			((struct rest_net_names_context *)request->response.context)->size = os_strlen(((struct rest_net_names_context *)request->response.context)->response);

			http_process_response(request->connection);
			
			//Free old data if there is any.
			for (i = 0; i < n_aps; i++)
			{
				db_free(ap_ssids[i]);
			}
			db_free(ap_ssids);
			ap_ssids = NULL;
			n_aps = 0;
		}
		else
		{
			//Get set if there is an error in the callback.
			error = false;
			//Prevent more scans until the call back has done it job.
			waiting_request = request;
			debug(" Starting scan.\n");
			if (wifi_station_scan(NULL, &scan_done_cb) && (!error))
			{
				debug(" Scanning for AP's.\n");
			}
			else
			{
				error(" Could not scan AP's.\n");
			}				
		}
	}
}

/**
 * @brief Generate the response for a HEAD request from a file.
 * 
 * @param request Request that got us here.
 * @return Return unused.
 */
size_t ICACHE_FLASH_ATTR rest_net_names_head_handler(struct http_request *request)
{
	char str_size[16];
	size_t ret = 0;
	
	//If the send buffer is over 200 bytes, this should never fill it.
	debug("REST network names HEAD handler.\n");
	if (!request->response.context)
    {
		request->response.context = db_zalloc(sizeof(struct rest_net_names_context), "request->response.context rest_names_get_handler");
	}
	switch(request->response.state)
	{
		case HTTP_STATE_STATUS:  //Start scanning until a response is ready.
								 if (!((struct rest_net_names_context *)request->response.context)->response)
								 {
									 create_response(request);
								 }
								 else
								 {
									 //Start sending the response.									 
									 ret = http_send_status_line(request->connection, request->response.status_code);
									 //Onwards
									 request->response.state = HTTP_STATE_HEADERS;
								 }
								 break;
		case HTTP_STATE_HEADERS: //Always send connections close and server info.
								 ret = http_send_header(request->connection, 
												  "Connection",
											      "close");
								 ret += http_send_header(request->connection,
												  "Server",
												  HTTP_SERVER_NAME);
								 //Get data size.
								 os_sprintf(str_size, "%d", ((struct rest_net_names_context *)request->response.context)->size);
								 //Send message length.
								 ret += http_send_header(request->connection, 
												  "Content-Length",
											      str_size);
								 ret += http_send_header(request->connection, "Content-Type", http_mime_types[MIME_JSON].type);	
								 //Send end of headers.
								 ret += http_send(request->connection, "\r\n", 2);
								 //Stop if only HEAD was requested.
								 if (request->type == HTTP_HEAD)
								 {
									 request->response.state = HTTP_STATE_ASSEMBLED;
								 }
								 else
								 {
									 request->response.state = HTTP_STATE_MESSAGE;
								 }
								 break;
	}
    return(ret);
}

/**
 * @brief Generate a JSON array of available access points.
 * 
 * @param uri The URI to answer.
 * @param request Data for the request that got us here.
 * @return The JSON.
 */
size_t ICACHE_FLASH_ATTR rest_net_names_get_handler(struct http_request *request)
{
	char *uri = request->uri;
	size_t ret;
	    
    debug("In network names REST handler (%s).\n", uri);

	//Don't duplicate, just call the head handler.
	if (request->response.state < HTTP_STATE_MESSAGE)
	{
		ret = rest_net_names_head_handler(request);
	}
	else
	{
		debug(" Response: %s.\n", ((struct rest_net_names_context *)request->response.context)->response);
		ret = http_send(request->connection, 
				 ((struct rest_net_names_context *)request->response.context)->response,
				 ((struct rest_net_names_context *)request->response.context)->size);
		request->response.state = HTTP_STATE_ASSEMBLED;
		
		debug(" Response size: %d.\n", ((struct rest_net_names_context *)request->response.context)->size);
	}
	return(ret);
}

/**
 * @brief Deallocate memory used for the response.
 * 
 * @param response A pointer to the mem to deallocate.
 */
void ICACHE_FLASH_ATTR rest_net_names_destroy(struct http_request *request)
{
	debug("Freeing network names REST handler data.\n");
	
	if (request->response.context)
	{	
		if (((struct rest_net_names_context *)request->response.context)->response)
		{
			db_free(((struct rest_net_names_context *)request->response.context)->response);
		}
		db_free(request->response.context);
	}
}
