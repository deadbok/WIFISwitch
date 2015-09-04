
/**
 * @file net-passwd.c
 *
 * @brief REST interface for setting the current network password.
 * 
 * Maps `/rest/net/password` to the password of the configures access point, but
 * only PUT method is supported.
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
#include "slighttp/http-handler.h"


//Set password for current network.
extern bool rest_net_passwd_test(struct http_request *request);
extern void rest_net_passwd_header(struct http_request *request, char *header_line);
extern signed int rest_net_passwd_put_handler(struct http_request *rquest);
extern void rest_net_passwd_destroy(struct http_request *request);


/**
 * @brief Struct used to register the handler.
 */
struct http_response_handler http_rest_passwd_handler =
{
	rest_net_passwd_test,
	rest_net_passwd_header,
	{
		NULL,
		NULL,
		NULL,
		NULL,
		rest_net_passwd_put_handler,
		NULL,
		NULL,
		NULL
	}, 
	rest_net_passwd_destroy
};

/**
 * @brief Keep track of actual password changes.
 */
static bool passwd_changed = false;
/**
 * @brief Tell if we will handle a certain URI.
 * 
 * @param request The request to handle.
 * @return True if we can handle the URI.
 */
bool ICACHE_FLASH_ATTR rest_net_passwd_test(struct http_request *request)
{
    if (os_strncmp(request->uri, "/rest/net/password\0", 19) == 0)
    {
		debug("Rest handler network found: %s.\n", request->uri);
        return(true);
    }
    debug("Rest handler network will not request,\n"); 
    return(false);       
}

/**
 * @brief Handle headers.
 * 
 * @param request The request to handle.
 * @param header_line Header line to handle.
 */
void ICACHE_FLASH_ATTR rest_net_passwd_header(struct http_request *request, char *header_line)
{
	debug("HTTP REST network password handler.\n");
}

/**
 * @brief Set the network password for the WIFI connection.
 * 
 * @param request Data for the request that got us here.
 * @return The HTML.
 */
signed int ICACHE_FLASH_ATTR rest_net_passwd_put_handler(struct http_request *request)
{
	struct jsonparse_state state;
	char *uri = request->uri;
	char passwd[64];
	int type;
	struct station_config sc;
	size_t ret = 0;
	static bool done = false;
	    
    debug("In network password PUT REST handler (%s).\n", uri);
    
    passwd_changed = false;
	if (request->response.state == HTTP_STATE_STATUS)
	{
		//Go on if we're done.
		if (done)
		{
			done = false;
			request->response.state++;
			return(0);
		}
		request->response.status_code = 204;
		ret = http_send_status_line(request->connection, request->response.status_code);
		done = true;
	}
	else if (request->response.state == HTTP_STATE_HEADERS)
	{
		//Go on if we're done.
		if (done)
		{
			done = false;
			request->response.state++;
			return(0);
		}
		ret = http_send_header(request->connection, "Connection", "close");
		ret += http_send_header(request->connection, "Server", HTTP_SERVER_NAME);
		ret += http_send(request->connection, "\r\n", 2);		
		done = true;
	}
	else if (request->response.state == HTTP_STATE_MESSAGE)
	{
		//Go on if we're done.
		if (done)
		{
			done = false;
			request->response.state++;
			return(0);
		}
		jsonparse_setup(&state, request->message, os_strlen(request->message));
		while ((type = jsonparse_next(&state)) != 0)
		{
			if (type == JSON_TYPE_PAIR_NAME)
			{
				if (jsonparse_strcmp_value(&state, "password") == 0)
				{
					debug(" Found network value in JSON.\n");
                    jsonparse_next(&state);
                    jsonparse_next(&state);
                    jsonparse_copy_value(&state, passwd, sizeof(passwd));
                    debug(" Password: %s.\n", passwd);

					wifi_station_get_config(&sc);
					
					sc.bssid_set = 0;
					//need not check MAC address of AP
				   
					os_memcpy(&sc.password, passwd, 64);
					wifi_station_set_config(&sc);
					passwd_changed = true;
				}
			}
		}
		request->response.message_size = 0;
		done = true;
	}
	return(ret);
}
/**
 * @brief Deallocate memory used for request.
 * 
 * @param request A pointer to the request with the context to deallocate.
 */
void ICACHE_FLASH_ATTR rest_net_passwd_destroy(struct http_request *request)
{
	debug("Freeing network password REST handler data.\n");
	
	if (request->response.context)
	{
		db_free(request->response.context);
	}
	if (passwd_changed)
	{
		//Restart to apply new WIFI configuration.
		debug(" System restart.\n");
		system_restart();
	}
}
