
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


/**
 * @brief Keep track of actual password changes.
 */
static bool passwd_changed = false;

/**
 * @brief REST handler to get/set the GPIO state and get GPIO information.
 *
 * @param request The request that we're handling.
 * @return Bytes send.
 */
signed int ICACHE_FLASH_ATTR http_rest_net_passwd_handler(struct http_request *request)
{
	int type;
	char passwd[64];
	signed int ret = 0;
	struct station_config sc;
	struct jsonparse_state json_state;
	
	if (!request)
	{
		warn("Empty request.\n");
		return(RESPONSE_DONE_ERROR);
	}
	if (request->type != HTTP_PUT)
	{
		debug(" File system handler only supports PUT.\n");
		return(RESPONSE_DONE_CONTINUE);
	}

    if (os_strncmp(request->uri, "/rest/net/password\0", 19) != 0)
    {
		debug("Rest handler network names will not request,\n"); 
        return(RESPONSE_DONE_CONTINUE);
    }
	
	if (request->response.state == HTTP_STATE_NONE)
	{
		if (request->type == HTTP_PUT)
		{
			request->response.status_code = 204;
			//Send status and headers.
			ret += http_send_status_line(request->connection, request->response.status_code);
			ret += http_send_default_headers(request, 0, NULL);
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
		if (request->type == HTTP_PUT)
		{
			jsonparse_setup(&json_state, request->message, os_strlen(request->message));
			while ((type = jsonparse_next(&json_state)) != 0)
			{
				if (type == JSON_TYPE_PAIR_NAME)
				{
					if (jsonparse_strcmp_value(&json_state, "password") == 0)
					{
						debug(" Found network value in JSON.\n");
						jsonparse_next(&json_state);
						jsonparse_next(&json_state);
						jsonparse_copy_value(&json_state, passwd, sizeof(passwd));
						debug(" Password: %s.\n", passwd);

						wifi_station_get_config(&sc);					
						sc.bssid_set = 0;
						os_memcpy(&sc.password, passwd, 64);
						wifi_station_set_config(&sc);
						passwd_changed = true;
					}
				}
			}
			request->response.message_size = 0;
			return(ret);
		}
	}
	    
	if (request->response.state == HTTP_STATE_DONE)
	{
		if (request->type == HTTP_GET)
		{
			debug("Freeing network password REST handler data.\n");	
			if (request->response.context)
			{
				db_free(request->response.context);
			}
		}
	}
	return(RESPONSE_DONE_FINAL);
}
