/** @file http-handler.c
 *
 * @brief Routines for request handler registration.
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
#include "http-handler.h"

/**
 * @brief Struct to keep info on a registered handler.
 */
struct http_handler_entry
{
	/**
	 * @brief Root URI that the handler will work with.
	 */
	char *uri;
	/**
	 * @brief Handler callbacks.
	 */
	 struct http_response_handler *handler;
	/**
     * @brief Pointers for the prev and next entry in the list.
     */
    DL_LIST_CREATE(struct http_handler_entry);
};

/**
 * @brief List to keep track of handlers.
 */
static struct http_handler_entry *response_handlers = NULL;
/**
 * @brief Number of registered handlers.
 */
static unsigned int n_handlers = 0;

/**
 * @brief Register a handler.
 * 
 * Register functions to handle some URIs. The list of registered
 * handlers is searched from the first added handler to the last.
 * Make sure to add the most specific URIs first.
 * 
 * @param uri The base URI that the handler will start at. Everything
 *            below this URI will be handled by this handler, except
 *            when another rule, *added before this*, will handle it.
 * @param handler Pointer to a http_handler struct with callbacks.
 */
bool ICACHE_FLASH_ATTR http_add_handler(char *uri, struct http_response_handler *handler)
{
	struct http_handler_entry *handlers = response_handlers;
	struct http_handler_entry *entry = NULL;
	size_t uri_size = os_strlen(uri);
	
	//Check for stuff that is a no go.
	debug("Adding URI handler %s.\n", uri);
	if (!handler)
	{
		debug("No handler.\n");
		return(false);
	}
	if (!uri)
	{
		debug("No URI.\n");
		return(false);
	}
	//Get mem for the entry in the list.
	entry = db_malloc(sizeof(struct http_handler_entry), "entry http_add_handler");
	//Add handler callbacks.
	entry->handler = handler;
	//Get mem for URI.
	entry->uri = db_malloc(uri_size + 1  * sizeof(char), "entry->uri http_add_handler");
	//Set URI.
	debug("Copying match URI, %d bytes.\n", uri_size);
	memcpy(entry->uri, uri, uri_size);
	entry->uri[uri_size] = '\0';
	
	if (handlers == NULL)
	{
		response_handlers = entry;
		entry->prev = NULL;
		entry->next = NULL;
	}
	else
	{
		DL_LIST_ADD_END(entry, handlers);
	}
	n_handlers++;
	debug("%d registered handlers.\n", n_handlers);
	return(true);
}

/**
 * @brief Remove a registered handler.
 * 
 * @param handler Pointer to a http_handler struct with callbacks.
 */
bool ICACHE_FLASH_ATTR http_remove_handler(struct http_response_handler *handler)
{
	struct http_handler_entry *entry = response_handlers;
	struct http_handler_entry *handlers = response_handlers;
	
	debug("Removing URI handler.\n");
	if (!handler)
	{
		debug("No handler.\n");
		return(false);
	}
	if (!handlers)
	{
		debug("No handlers registered.\n");
		return(false);
	}
	
	while (handlers)
	{
		if (handlers->handler == handler)
		{
			debug("Unlinking handler.\n");
			DL_LIST_UNLINK(entry, handlers);	
			debug("Deallocating URI.\n");
			db_free(entry->uri);
			debug("Deallocating handler info.\n");
			db_free(entry);
			n_handlers--;
			debug("%d registered handlers.\n", n_handlers);
			return(true);
		}
		handlers = handlers->next;
	}
	warn(" Handler not fount.\n");
	return(false);
}

/**
 * @brief Find a handler for an URI.
 * 
 * @param request Pointer to the request data, only URI needs to be populated.
 * @return Pointer to a handler struct, or NULL if not found.
 */
struct http_response_handler ICACHE_FLASH_ATTR *http_get_handler(struct http_request *request)
{
	struct http_handler_entry *handlers = response_handlers;
	unsigned short i = 1;
	size_t handler_uri_length;
	
	debug("Finding handler.\n");
	if (!request)
	{
		debug(" No request data.\n");
		return(NULL);
	}
	if (!request->uri)
	{
		debug(" No URI.\n");
		return(NULL);
	}
	debug(" URI: %s.\n", request->uri);
	if (!handlers)
	{
		debug(" No handlers.\n");
		return(NULL);
	}
	
	debug(" %d response handlers.\n", n_handlers);
	//Check URI, go through and stop if URIs match.
	while (handlers)
	{
		debug(" Trying handler %d, URI: %s.\n", i, handlers->uri);
		handler_uri_length = os_strlen(handlers->uri);
		if (handlers->uri[handler_uri_length - 1] != '*')
		{
			debug(" Using strict matching.\n");
			if (handler_uri_length != os_strlen(request->uri))
			{
				//Signal that length does not match.
				debug(" URI length is not equal.\n");
				handler_uri_length = 0;
			}
		}
		else
		{
			//Dont't use the '*'.
			handler_uri_length--;
		}
		if (handler_uri_length)
		{
			if (os_strncmp(handlers->uri, request->uri,
						   handler_uri_length) == 0)
			{
				debug(" URI handlers for %s at %p.\n", request->uri,
					  handlers->handler);
				return(handlers->handler);
			}
		}
		i++;
		handlers = handlers->next;
	}
	debug(" No response handler found for URI %s.\n", request->uri);
	return(NULL);
}

/**
 * @brief Handle headers.
 * 
 * Default handler for headers, that does nothing.
 * 
 * @param request The request to handle.
 * @param header_line Header line to handle.
 */
void ICACHE_FLASH_ATTR http_default_header_handler(struct http_request *request, char *header_line)
{
	debug("HTTP ignoring header \"%s\".\n", header_line);
}
