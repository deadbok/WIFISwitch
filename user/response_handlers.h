/** @file response_handlers.h
 *
 * @brief Main state machine type definitions.
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
#ifndef RESPONSE_HANDLERS_H
#define RESPONSE_HANDLERS_H

#include "slighttp/http.h"
#include "rest/rest.h"

/**
 * @brief Number of response handlers.
 */
#define N_RESPONSE_HANDLERS  3

/**
 * @brief Array of built in handlers and their URIs.
 */
static struct http_response_handler response_handlers[N_RESPONSE_HANDLERS] =
{
	{
		http_fs_test,
		{
			NULL,
			http_fs_get_handler,
			http_fs_head_handler,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
		}, 
		http_fs_destroy
	},
	{
		rest_network_test,
		{
			NULL,
			rest_network_get_handler,
			rest_network_head_handler,
			NULL,
			rest_network_put_handler,
			NULL,
			NULL,
			NULL
		}, 
		rest_network_destroy
	},
    {
		rest_net_names_test,
		{
			NULL,			 
			rest_net_names_get_handler,
			rest_net_names_head_handler,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
		}, 
		rest_net_names_destroy
	}
};

#endif //RESPONSE_HANDLERS_H
