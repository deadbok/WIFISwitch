/**
 * @file sm_wifi.c
 *
 * @brief WIFI code for the main state machine.
 *
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@@ace2>
 * 
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
#include "user_interface.h"
#include "slighttp/http.h"
#include "sm_types.h"

/**
 * @brief Connect to the configured Access Point, or create an AP, if unsuccessful.
 * 
 * This will keep calling itself through the main state machine until a
 * connection is made, or it times out, and sets up an access point.
 */
state_t ICACHE_FLASH_ATTR http_init(void *arg)
{
	struct sm_context *context = arg;
	
	if (init_http(context->http_root, NULL, 0))
	{
		error("Could not start HTTP server.\n");
		return(SYSTEM_RESTART);
	}
	return(HTTP_SEND);
}

/**
 * @brief Do house keeping.
 * 
 * Close and deallocate unused connections.
 */
state_t http_mutter_med_kost_og_spand(void *arg)
{
	return(HTTP_SEND);
}
