
/**
 * @file http-deny.c
 *
 * @brief Deny access to URI's.
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
#include "tools/strxtra.h"
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "slighttp/http-response.h"
#include "handlers/deny/http-deny.h"

bool http_deny_test(struct http_request *request);
void http_deny_header(struct http_request *request, char *header_line);
signed int http_deny_all_handler(struct http_request *request);
void http_deny_destroy(struct http_request *request);

/**
 * @brief Struct used to register the handler.
 */
struct http_response_handler http_deny_handler =
{
	http_deny_test,
	http_deny_header,
	{
		http_deny_all_handler,
		http_deny_all_handler,
		http_deny_all_handler,
		http_deny_all_handler,
		http_deny_all_handler,
		http_deny_all_handler,
		http_deny_all_handler,
		http_deny_all_handler
	}, 
	http_deny_destroy
};

/**
 * @brief Tell if we will handle a certain URI.
 * 
 * @param request The request to handle.
 * @return True if we can handle the URI.
 */
bool ICACHE_FLASH_ATTR http_deny_test(struct http_request *request)
{
	//Handle anything, but return 403.
    return(true); 
}

/**
 * @brief Handle headers.
 * 
 * @param request The request to handle.
 * @param header_line Header line to handle.
 */
void ICACHE_FLASH_ATTR http_deny_header(struct http_request *request, char *header_line)
{
	debug("HTTP deny header handler.\n");
}

/**
 * @brief Generate the response for a HEAD request from a file.
 * 
 * @param request Request that got us here.
 * @return Return unused.
 */
signed int ICACHE_FLASH_ATTR http_deny_all_handler(struct http_request *request)
{
	request->response.status_code = 403;
	request->response.state = HTTP_STATE_ERROR;
	return(0);
}

/**
 * @brief Deallocate memory used for the response.
 * 
 * @param request Request for which to free the response.
 */
void ICACHE_FLASH_ATTR http_deny_destroy(struct http_request *request)
{
	debug("Freeing data for deny response.\n");
}
