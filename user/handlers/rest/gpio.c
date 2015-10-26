/**
 * @file gpio.c
 *
 * @brief REST interface for GPIO control.
 * 
 * Maps `/rest/gpios` to give general info on GPIO's.
 *  - GET Spits out an array of ints with enabled GPIO's
 * 
 * Maps '/rest/gpios/N to a specific GPIO pin.
 * - GET Returns an object with a member "state" being 0 or 1.
 * - PUT Set the state of the GPIO with an object containing member "state" with a value of 0 or 1.
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
#include <ctype.h>
#include <stdlib.h>
#include "c_types.h"
#include "eagle_soc.h"
#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"
#include "user_config.h"
#include "tools/jsmn.h"
#include "tools/json-gen.h"
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "slighttp/http-response.h"
#include "slighttp/http-handler.h"
#include "tools/strxtra.h"
#include "user_config.h"

#ifndef REST_GPIO_ENABLED
#warning "No GPIO's has been enabled for access via the REST interface."
/**
 * @brief Bit mask of which GPIO pins the REST interface can control.
 */
#define REST_GPIO_ENABLED 0
#endif

/**
 * @brief Number of GPIO's
 */
#define REST_GPIO_PINS 16
/**
 * @brief GPIO were are working with or -1 if no specific pin.
 */
static signed char current_gpio = -1;

/**
 * @brief Create a JSON array containing all usable GPIO's.
 * 
 * @param request Request that got us here.
 * @return Size of response.
 */
static signed int create_enabled_response(struct http_request *request)
{
	char json_value[3];
	char *json_response = NULL;
	
	debug("Creating JSON array of enabled GPIO's.\n");
	//Run through all GPIO's
	for (unsigned int i = 0; i < REST_GPIO_PINS; i++)
	{
		if (((REST_GPIO_ENABLED >> i) & 1) == 1)
		{
			debug(" GPIO%d is enabled.\n", i);
			if (!itoa(i, json_value, 10))
			{
				warn("Could not convert GPIO pin number to string.\n");
			}
			json_response = json_add_to_array(json_response, json_value);
		}
	}
	request->response.context = json_response;
	
	return(os_strlen(request->response.context));
}

/**
 * @brief Create a JSON response with the state of a specific GPIO.
 * 
 * @param request Request that got us here.
 * @return Size of response.
 */
static signed int create_pin_response(struct http_request *request)
{
	unsigned char state;

	debug("Getting state of GPIO%d.\n", current_gpio);
	
	state = GPIO_INPUT_GET(current_gpio);
	
	debug(" GPIO state: %d.\n", state);
	
	request->response.context = db_malloc(sizeof(char) * 12,
										  "request->response.context->response create_pin_response");
	if (state)
	{
		os_memcpy(((char *)(request->response.context)), "{\"state\":1}", 11);
	}
	else
	{
		os_memcpy(((char *)(request->response.context)), "{\"state\":0}", 11);
	}
	((char *)(request->response.context))[11] = '\0';
	return(11);
}

/**
 * @brief REST handler to get/set the GPIO state and get GPIO information.
 *
 * @param request The request that we're handling.
 * @return Bytes send.
 */
signed int http_rest_gpio_handler(struct http_request *request)
{
	size_t size = 0;
	signed int ret = 0;
	unsigned int i;
	unsigned int gpio_state;
		
	if (!request)
	{
		warn("Empty request.\n");
		return(RESPONSE_DONE_ERROR);
	}
	if ((request->type != HTTP_GET) &&
		(request->type != HTTP_HEAD) &&
		(request->type != HTTP_PUT))
	{
		debug(" Rest GPIO handler only supports HEAD, GET, and PUT.\n");
		return(RESPONSE_DONE_CONTINUE);
	}
    if (os_strncmp(request->uri, "/rest/gpios", 11) == 0)
    {
		if (request->uri[11] == '/')
		{
			if (isdigit((int)request->uri[12]))
			{
				current_gpio = atoi(request->uri + 12);
				//Check if GPIO is enabled.
				if (((REST_GPIO_ENABLED >> current_gpio) & 1) == 1)
				{
					debug("Rest handler GPIO%d found: %s.\n", current_gpio, request->uri);				
				}
				else
				{
					debug("Rest handler GPIO will not handle request, pin %d not enabled.\n", current_gpio);
					return(RESPONSE_DONE_CONTINUE);
				}
			}
			else
			{
				debug("Rest handler GPIO will not handle request.\n");
				return(RESPONSE_DONE_CONTINUE);
			}				
		}
		else if (request->uri[11] == '\0')
		{
			debug("Rest handler GPIO (global) found: %s.\n", request->uri);
			current_gpio = -1;
		}
		else
		{
			debug("Rest handler GPIO (global) will not handle request.\n");
			return(RESPONSE_DONE_CONTINUE);
		}
    }
	
	if (request->response.state == HTTP_STATE_NONE)
	{
		if ((request->type == HTTP_HEAD) ||
		    (request->type == HTTP_GET))
		{
			//Get data size and create response.
			if (current_gpio < 0)
			{
				size = create_enabled_response(request);
			}
			else
			{
				size = create_pin_response(request);
			}
			//Send status and headers.
			ret += http_send_status_line(request->connection, request->response.status_code);
			ret += http_send_default_headers(request, size, "json");
			if (request->type == HTTP_HEAD)
			{
				request->response.state = HTTP_STATE_DONE;
				return(ret);
			}
			else
			{
				//Go on to sending the file.
				request->response.state = HTTP_STATE_MESSAGE;
			}
		}
		else
		{
			//Handle trying to PUT to /rest/gpios
			if (current_gpio < 0)
			{
				request->response.status_code = 405;
				ret = http_send_status_line(request->connection, request->response.status_code);
			}
			else
			{
				request->response.status_code = 204;
				ret = http_send_status_line(request->connection, request->response.status_code);
			}
			//We have not send anything.
			request->response.message_size = 0;
			//Send status and headers.
			ret += http_send_status_line(request->connection, request->response.status_code);
			ret += http_send_default_headers(request, 0, NULL);
			//Go on to set the network.
			request->response.state = HTTP_STATE_MESSAGE;
		}
	}
	
	if (request->response.state == HTTP_STATE_MESSAGE)
	{
		if (request->type == HTTP_GET)
		{
			size = os_strlen(request->response.context);
			debug(" Response: %s.\n", ((char *)(request->response.context)));
			ret = http_send(request->connection, ((char *)(request->response.context)), size);
			request->response.state = HTTP_STATE_DONE;
			request->response.message_size = size;
		}
		else
		{
			//Handle trying to PUT to /rest/gpios
			if (current_gpio < 0)
			{
				ret += http_send(request->connection, "<!DOCTYPE html><head><title>Method Not Allowed.</title></head><body><h1>405 Method Not Allowed.</h1><br />I won't PUT up with this.</body></html>", 145);
				request->response.message_size = ret;
				request->response.state = HTTP_STATE_DONE;
			}
			else
			{
				debug(" GPIO selected: %s.\n", request->message);
				jsmn_parser parser;
				jsmntok_t tokens[3];
				int n_tokens;
			
				jsmn_init(&parser);
				n_tokens  = jsmn_parse(&parser, request->message, os_strlen(request->message), tokens, 3);
				//We expect 3 tokens in an object.
				if ( (n_tokens < 3) || tokens[0].type != JSMN_OBJECT)
				{
					warn("Could not parse JSON request.\n");
					return(RESPONSE_DONE_ERROR);				
				}
				for (i = 1; i < n_tokens; i++)
				{
					debug(" JSON token %d.\n", i);
					if (tokens[i].type == JSMN_STRING)
					{
						debug(" JSON token start with a string.\n");
						if (strncmp(request->message + tokens[i].start, "state", 5) == 0)
						{
							i++;
							if (tokens[i].type == JSMN_PRIMITIVE)
							{
								debug(" JSON primitive comes next.\n");
								if (isdigit((int)*(request->message + tokens[i].start)) || (*(request->message + tokens[i].start) == '-'))
								{
									gpio_state = atoi(request->message + tokens[i].start);
									debug(" State: %d.\n", gpio_state);
									GPIO_OUTPUT_SET(current_gpio, gpio_state);
								}
							}
						}
					}
				}
			}
		}
		return(ret);
	}
	    
	if (request->response.state == HTTP_STATE_DONE)
	{
		if (request->type == HTTP_GET)
		{
			debug("Freeing GPIO REST handler data.\n");
			if (request->response.context)
			{
				debug(" Freeing context.\n");
				db_free(request->response.context);
				request->response.context = NULL;
			}
		}
	}
	return(RESPONSE_DONE_FINAL);
}
