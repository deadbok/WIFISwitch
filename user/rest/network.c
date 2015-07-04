
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
#include "json/jsonparse.h"
#include "user_config.h"
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "slighttp/http-response.h"

/**
 * @brief Tell if we will handle a certain URI.
 * 
 * @param request The request to handle.
 * @return True if we can handle the URI.
 */
bool ICACHE_FLASH_ATTR rest_network_test(struct http_request *request)
{
    if (os_strncmp(request->uri, "/rest/net/network\0", 18) == 0)
    {
		debug("Rest handler network found: %s.\n", request->uri);
        return(true);
    }
    debug("Rest handler network will not request,\n"); 
    return(false);       
}

/**
 * @brief Create the response.
 * 
 * @param request Request to respond to..
 * @return Size of the response.
 */
static size_t create_response(struct http_request *request)
{
	struct station_config wifi_config;
	char *response;
	char *response_pos;
	    
    debug("Creating network REST response.\n");
	
	response = db_malloc(sizeof(char) * 51, "response create_response");
	response_pos = response;
	
	os_strcpy(response, "{ \"network\" : \"");
	response_pos += 15;

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
	
	request->response.context = response;
	
	return(response_pos - response);
}

/**
 * @brief Generate the response for a HEAD request from a file.
 * 
 * @param request Request that got us here.
 * @return Return unused.
 */
size_t ICACHE_FLASH_ATTR rest_network_head_handler(struct http_request *request)
{
	char str_size[16];
	size_t size;
	
	//If the send buffer is over 200 bytes, this should never fill it.
	debug("REST network HEAD handler.\n");
	size = create_response(request);
	switch(request->response.state)
	{
		case HTTP_STATE_STATUS:  http_send_status_line(request->connection, request->response.status_code);
								 //Onwards
								 request->response.state = HTTP_STATE_HEADERS;
								 break;
		case HTTP_STATE_HEADERS: //Always send connections close and server info.
								 http_send_header(request->connection, 
												  "Connection",
											      "close");
								 http_send_header(request->connection,
												  "Server",
												  HTTP_SERVER_NAME);
								 //Get data size.
								 os_sprintf(str_size, "%d", size);
								 //Send message length.
								 http_send_header(request->connection, 
												  "Content-Length",
											      str_size);
								 http_send_header(request->connection, "Content-Type", http_mime_types[MIME_JSON].type);	
								 //Send end of headers.
								 http_send(request->connection, "\r\n", 2);
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
    return(0);
}

/**
 * @brief Generate the HTML for configuring the network connection.
 * 
 * @param request Data for the request that got us here.
 * @return Size of the return massage.
 */
size_t ICACHE_FLASH_ATTR rest_network_get_handler(struct http_request *request)
{
	size_t msg_size = 0;
	char *uri = request->uri;
	    
    debug("In network name GET REST handler (%s).\n", uri);
	
	//Don't duplicate, just call the head handler.
	if (request->response.state < HTTP_STATE_MESSAGE)
	{
		rest_network_head_handler(request);
	}
	else
	{
		msg_size = os_strlen(request->response.context);
		debug(" Response: %s.\n", (char *)request->response.context);
		tcp_send(request->connection, request->response.context, msg_size);
		request->response.state = HTTP_STATE_ASSEMBLED;
		
		debug(" Response size: %d.\n", msg_size);
		return(msg_size);
	}
	return(0);
}

/**
 * @brief Set the network name for the WIFI connection.
 * 
 * @param request Data for the request that got us here.
 * @return The HTML.
 */
size_t ICACHE_FLASH_ATTR rest_network_put_handler(struct http_request *request)
{
	struct jsonparse_state state;
	char *uri = request->uri;
	char net_name[32];
	int type;
	struct station_config sc;
	    
    debug("In network name PUT REST handler (%s).\n", uri);
    
	if (request->response.state == HTTP_STATE_STATUS)
	{
		http_send_status_line(request->connection, 204);
		debug(" Network selected: %s.\n", request->message);
		
		jsonparse_setup(&state, request->message, os_strlen(request->message));
		while ((type = jsonparse_next(&state)) != 0)
		{
			if (type == JSON_TYPE_PAIR_NAME)
			{
				if (jsonparse_strcmp_value(&state, "network") == 0)
				{
					debug(" Found network value in JSON.\n");
                    jsonparse_next(&state);
                    jsonparse_next(&state);
                    jsonparse_copy_value(&state, net_name, sizeof(net_name));
                    debug(" Name: %s.\n", net_name);

					sc.bssid_set = 0;
					//need not check MAC address of AP
				   
					os_memcpy(&sc.ssid, net_name, 32);
					sc.password[0] = '\0';
					wifi_station_set_config(&sc);
				}
			}
		}
		request->response.state = HTTP_STATE_ASSEMBLED;
	}
	return(0);
}
/**
 * @brief Deallocate memory used for the HTML.
 * 
 * @param html A pointer to the mem to deallocate.
 */
void ICACHE_FLASH_ATTR rest_network_destroy(struct http_request *request)
{
	if (request->response.context)
	{
		db_free(request->response.context);
	}
}
