
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

/**
 * @brief Generate the response for a HEAD request from a file.
 * 
 * @param request Request that got us here.
 * @return Return unused.
 */
signed int ICACHE_FLASH_ATTR http_deny_handler(struct http_request *request)
{
	debug("Denying access to %s.\n", request->uri);
	request->response.status_code = 403;
	return(RESPONSE_DONE_CONTINUE);
}

