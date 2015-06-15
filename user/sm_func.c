/**
 * @file sm_func.c
 * 
 * @brief Main state machine functions.
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
#include "user_config.h"
#include "sm_types.h"

/**
 * @brief Set the root of the HTTP server for config mode.
 */
state_t root_config_mode(void *arg)
{
	struct sm_context *context = arg;
	
	context->http_root = "/connect/";
	debug("Setting HTTP root to: %s.\n", context->http_root);
	return(HTTP_INIT);
}

/**
 * @brief Set the root of the HTTP server for normal mode.
 */
state_t root_normal_mode(void *arg)
{
	struct sm_context *context = arg;
	
	context->http_root = "/";
	debug("Setting HTTP root to: %s.\n", context->http_root);
	return(HTTP_INIT);
}

/**
 * @brief Restart the system.
 */
state_t reboot(void *arg)
{
	debug("Restarting device...\n");
	system_restart();
	return(SYSTEM_RESTART);
}
