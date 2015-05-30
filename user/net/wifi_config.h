/**
 * @file wifi_config.h
 *
 * @brief Routines for showing an AP configuration page.
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
#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include "c_types.h"

extern bool wifi_conf_test(char *uri);
extern char *wifi_conf_html(char *uri, struct http_request *request);
extern void wifi_conf_destroy(char *html);

#endif //WIFI_CONFIG_H
