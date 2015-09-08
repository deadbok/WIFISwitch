/** @file rest.h
 *
 * @brief REST interface call backs.
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
#ifndef REST_H
#define REST_H

#include "slighttp/http-handler.h"

/**
 * @brief REST handler for the default network name.
 */
extern signed int http_rest_network_handler(struct http_request *request);

/**
 * @brief REST handler for the scanning for available networks.
 */
extern signed int http_rest_net_names_handler(struct http_request *request);

/**
 * @brief REST handler for setting the password for the default network.
 */
extern signed int http_rest_net_passwd_handler(struct http_request *request);

/**
 * @brief REST handler for GPIO toggling.
 */
extern signed int http_rest_gpio_handler(struct http_request *request);

//General REST function.
extern bool rest_init(void);

#endif
