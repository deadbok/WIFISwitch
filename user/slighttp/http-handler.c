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
#include "tools/strxtra.h"
#include "http-response.h"
#include "http-handler.h"
#include "http-mime.h"

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
	http_handler_callback handler;
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
 * @param handler Pointer to the callback.
 */
bool http_add_handler(char *uri, http_handler_callback handler)
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
 * @param handler Pointer to a the callback.
 */
bool http_remove_handler(http_handler_callback handler)
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
 * @param start_handler If not NULL, search the list only after this handler-
 * @return Function pointer to a handler.
 */
http_handler_callback http_get_handler(
	struct http_request *request,
	http_handler_callback start_handler
)
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
	if (start_handler != NULL)
	{
		debug(" Starting at handler %p.\n", start_handler);
		while (handlers)
		{
			if (handlers->handler == start_handler)
			{
				debug(" Handler %p is number %d.\n", start_handler, i);
				i++;
				if (i > n_handlers)
				{
					debug(" No more handlers.\n");
					return(NULL);
				}
				handlers = handlers->next;
				break;
			}
			i++;
			handlers = handlers->next;
		}
	}
	
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
 * @brief Last chance status handler.
 * 
 * @param request Request to handle.
 * @return Bytes send.
 */
signed int http_status_handler(struct http_request *request)
{
	size_t size = 0;
	char str_size[5];
	char *msg = NULL;
	char default_msg[102];
	signed int ret;
	static bool done = false;
	
	if (!request)
	{
		warn("Empty request.\n");
		return(RESPONSE_DONE_ERROR);
	}
	if (done)
	{
		done = false;
		return(RESPONSE_DONE_FINAL);
	}

	debug("Last chance handler status code %d.\n", request->response.status_code);
	//We only get here if there was no one else willing to do the job.
	if (request->response.status_code < 400)
	{
		debug(" Returning 404.\n");
		request->response.status_code = 404;
	}
	switch (request->response.status_code)
	{
		case 400:
			size = HTTP_400_HTML_LENGTH;
			msg = HTTP_400_HTML;
			break;
		case 403:
			size = HTTP_403_HTML_LENGTH;
			msg = HTTP_403_HTML;
			break;
		case 404:
			size = HTTP_404_HTML_LENGTH;
			msg = HTTP_404_HTML;
			break;
		case 405:
			size = HTTP_405_HTML_LENGTH;
			msg = HTTP_405_HTML;
			break;
		case 500:
			size = HTTP_500_HTML_LENGTH;
			msg = HTTP_500_HTML;
			break;
		case 501:
			size = HTTP_501_HTML_LENGTH;
			msg = HTTP_501_HTML;
			break;
		default:
			os_memcpy(default_msg, HTTP_ERROR_HTML_START, 83);
			size = 83;
			itoa(request->response.status_code, (char *)(default_msg + size), 10);
			size += 3;
			os_memcpy(default_msg + size, HTTP_ERROR_HTML_END "\0", 16);
			size += 15;
			msg = default_msg;
	}
	itoa(size, str_size, 10);
	ret = http_send_status_line(request->connection, request->response.status_code);
	ret += http_send_header(request->connection, "Connection", "close");
	ret += http_send_header(request->connection, "Server", HTTP_SERVER_NAME);	
	//Send message length.
	ret += http_send_header(request->connection, "Content-Length", str_size);
	ret += http_send_header(request->connection, "Content-Type", http_mime_types[MIME_HTML].type);	
	//Send end of headers.
	ret += http_send(request->connection, "\r\n", 2);
	if (msg)
	{
		ret += http_send(request->connection, msg, size);
	}
	request->response.message_size = ret;
	done = true;
	return(ret);
}


/**
 * @brief Template handler for simple GET/PUT handlers.
 * 
 * A simple handler template, that support GET and PUT. Messages in
 * request->response.message, is send after the respective callback.
 * No more than about 1300 bytes can be send.
 *
 * If any callback pointer is NULL, a request with that method is 
 * skipped.
 * 
 * @param request The request that we're handling.
 * @param get_cb Callback to handle get requests.
 * @param put_cb Callback to handle put requests
 * @return Bytes send.
 */
signed int http_simple_GET_PUT_handler(
	struct http_request *request, 
	http_handler_callback get_cb,
	http_handler_callback put_cb,
	http_handler_callback free_cb
)
{
	signed int ret = 0;
	size_t msg_size = 0;
		
	if (!request)
	{
		warn("Empty request.\n");
		return(RESPONSE_DONE_ERROR);
	}
	if (((request->type == HTTP_GET) ||
		(request->type == HTTP_HEAD)) &&
		(get_cb == NULL))
	{
		debug(" Simple GET PUT handler will not handle HEAD/GET request.\n");
		return(RESPONSE_DONE_CONTINUE);
	}
	if ((request->type == HTTP_PUT) &&
		(put_cb == NULL))
	{
		debug(" Simple GET PUT handler will not handle PUT request.\n");
		return(RESPONSE_DONE_CONTINUE);
	}
	
	if (request->response.state == HTTP_STATE_NONE)
	{
		debug("Simple GET PUT handler entering state %d.\n", request->response.state);
		if ((request->type == HTTP_HEAD) ||
		    (request->type == HTTP_GET))
		{
			msg_size = get_cb(request);
		}
		else
		{
			msg_size = put_cb(request);
		}
		//No data.
		if (msg_size == 0)
		{
			request->response.status_code = 204;
		}
		//We have not send anything.
		request->response.message_size = 0;
		//Send status and headers.
		ret += http_send_status_line(request->connection, request->response.status_code);
		ret += http_send_default_headers(request, msg_size, "json");
		if (request->type == HTTP_HEAD)
		{
			debug("Simple GET PUT handler leaving state %d.\n", request->response.state);
			request->response.state = HTTP_STATE_DONE;
			return(ret);
		}
		else
		{
			//Go on to sending the file.
			request->response.state = HTTP_STATE_MESSAGE;
		}
	}
	
	if (request->response.state == HTTP_STATE_MESSAGE)
	{
		debug("Simple GET PUT handler entering state %d.\n", request->response.state);
		if (request->response.message)
		{
			msg_size = os_strlen(request->response.message);
			debug(" Response: %s.\n", (char *)request->response.message);
			ret += http_send(request->connection, request->response.message, msg_size);
		}
		//We're done sending the message.
		request->response.state = HTTP_STATE_DONE;
		request->response.message_size = msg_size;
		debug("Simple GET PUT handler leaving state %d.\n", request->response.state);
		return(ret);
	}
	    
	if (request->response.state == HTTP_STATE_DONE)
	{
		debug("Simple GET PUT handler entering state %d.\n", request->response.state);
		if (free_cb)
		{
			debug(" Freeing request data.\n");
			free_cb(request);
		}
		if (request->response.context)
		{
			debug(" Freeing context data.\n");
			db_free(request->response.context);
			request->response.context = NULL;
		}
	}
	debug("Simple GET PUT handler leaving state %d.\n", request->response.state);
	return(RESPONSE_DONE_FINAL);
}
