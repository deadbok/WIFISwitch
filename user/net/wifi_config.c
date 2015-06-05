/**
 * @file wifi_config.c
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
#include "c_types.h"
#include "eagle_soc.h"
#include "osapi.h"
#include "tools/missing_dec.h"
#include "tools/strxtra.h"
#include "slighttp/http.h"
#include "fs/fs.h"

/**
 * @brief Tell if we will handle a certain URI.
 * 
 * @param uri The URI to test.
 * @return True if we can handle the URI.
 */
bool ICACHE_FLASH_ATTR wifi_conf_test(char *uri)
{
    if (os_strncmp(uri, "/connect/index.html", 19) == 0)
    {
        return(true);
    }
    return(false);  
}

/**
 * @brief Generate the HTML for configuring the network connection.
 * 
 * @param uri The URI to answer.
 * @param request Data for the request that got us here.
 * @return The HTML.
 */
char ICACHE_FLASH_ATTR *wifi_conf_html(char *uri, struct http_request *request)
{
    char *html = NULL;
    
    debug("In network config page generator (%s).\n", uri);
 
    return(html);
}

/**
 * @brief Deallocate memory used for the HTML.
 * 
 * @param html A pointer to the mem to deallocate.
 */
void ICACHE_FLASH_ATTR wifi_conf_destroy(char *html)
{
	debug("Freeing static network config data.\n");
}
