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
#include "tmpl/tmpl.h"

/**
 * @brief Template context for the page.
 */
static struct tmpl_context *wifi_conf_context = NULL;
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
    char *tmpl = NULL;
    FS_FILE_H file;
    size_t	size;
    char *tmpl_uri = NULL;
    
    debug("In network config page generator (%s).\n", uri);
    
    //Change extension.
    size = os_strlen(uri);
    tmpl_uri = db_malloc(size, "tmpl_uri wifi_conf_html");
    tmpl_uri = os_memcpy(tmpl_uri, uri, size - 1);
    tmpl_uri = strrpl(tmpl_uri, ".tmpl", size - 5);
    tmpl_uri[size] = '\0';
    
    //Try to open the URI as a file.
    file = fs_open(tmpl_uri);
    //Check if it went okay.
    if (file > -1)
    {
        //Get the size of the message.
        size = fs_size(file);
        //Get memory for the message.
        tmpl = db_malloc((int)size + 1, "html wifi_conf_html");
        //Read the message from the file to the buffer.
        fs_read(tmpl, size, sizeof(char), file);
        //Zero terminate, to make sure.
        tmpl[size] = '\0';
        fs_close(file);        
    }
	else
	{
		error("Could not open template.\n");
		return(NULL);
	}

    db_free(tmpl_uri);
    
    //Run through the template engine.
    wifi_conf_context = init_tmpl(tmpl);
    //Add variables to template.
    tmpl_add_var(wifi_conf_context, "network", SSID_PASSWORD, TMPL_STRING);
    //Render template.
    html = tmpl_apply(wifi_conf_context);
    
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
	tmpl_destroy(wifi_conf_context);
	db_free(html);
}
