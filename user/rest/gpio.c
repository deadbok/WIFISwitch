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
#include "json/jsonparse.h"
#include "user_config.h"
#include "slighttp/http.h"
#include "slighttp/http-mime.h"
#include "slighttp/http-response.h"
#include "tools/strxtra.h"
#include "user_config.h"

#ifndef REST_GPIO_ENABLED
#warning "No GPIO's has been enabled for acces via the REST interface."
#define REST_GPIO_ENABLED = 0
#endif

struct rest_net_names_context
{
	/**
	 * @brief The response.
	 */
	char *response;
	/**
	 * @brief The number of bytes to send.
	 */
	size_t size;
};

/**
 * @brief GPIO were are working with or -1 if no specific pin.
 */
static signed char current_gpio = -1;

/**
 * @brief Tell if we will handle a certain URI.
 * 
 * @param request The request to handle.
 * @return True if we can handle the URI.
 */
bool ICACHE_FLASH_ATTR rest_gpio_test(struct http_request *request)
{
	bool ret = false;
	
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
					ret = true;
				}
			}
		}
		else if (request->uri[11] == '\0')
		{
			debug("Rest handler GPIO (global) found: %s.\n", request->uri);
			current_gpio = -1;
			ret = true;
		}
    }
    if (!ret)
    {
		debug("Rest handler GPIO will not handle request,\n"); 
	}
    return(ret);       
}

/**
 * @brief Create a JSON array of integers.
 * 
 * @param Pointer to an array of long ints.
 * @param Number of entries in the array.
 * @return Pointer to the JSON array.
 */
static char ICACHE_FLASH_ATTR *json_create_int_array(long *values, size_t entries)
{
	size_t i;
	size_t total_length = 0;
	size_t *lengths = db_malloc(sizeof(size_t) * entries, "lengths json_create_string_array");
	char *ret, *ptr;
	
	//'['
	total_length++;
	for (i = 0; i < entries; i++)
	{
		lengths[i] = digits(values[i]);
		total_length += lengths[i];
		//","
		if (i != (entries -1))
		{
			total_length++;
		}
	}
	//']'
	total_length++;
	
	ret = (char *)db_malloc(total_length, "res json_create_string_array");
	ptr = ret;
	
	*ptr++ = '[';
	for (i = 0; i < entries; i++)
	{
		itoa(values[i], ptr, 10);
		ptr += lengths[i] - 1;
		//","
		if (i != (entries -1))
		{
			*ptr++ = ',';
		}
	}
	*ptr++ = ']';
	*ptr = '\0';
	
	db_free(lengths);
	
	return(ret);
}

/**
 * @brief Create a JSON array containing all usable GPIO's.
 * 
 * @param request Request that got us here.
 */
static size_t create_enabled_response(struct http_request *request)
{
	unsigned char n_gpios = 0;
	long gpios[16];
	
	debug("Creating JSON array of enabled GPIO's.\n");
	//Run through all GPIO's
	for (unsigned int i = 0; i < 16; i++)
	{
		if (((REST_GPIO_ENABLED >> i) & 1) == 1)
		{
			debug(" GPIO%d is enabled.\n", i);
			gpios[n_gpios++] = i;
		}
	}
	request->response.context = json_create_int_array(gpios, n_gpios);
	
	return(os_strlen(request->response.context));
}

/**
 * @brief Create a JSON response with the state of a specific GPIO.
 * 
 * @param request Request that got us here.
 */
static size_t create_pin_response(struct http_request *request)
{
	unsigned char state;
	
	debug("Getting state of GPIO%d.\n", current_gpio);
	
	state = GPIO_INPUT_GET(current_gpio);
	
	debug(" GPIO state: %d.\n", state);
	
	request->response.context = db_malloc(sizeof(char) * 12, "request->response.context create_pin_response");
	
	if (state)
	{
		os_memcpy(request->response.context, "{\"state\":1}", 11);
	}
	else
	{
		os_memcpy(request->response.context, "{\"state\":0}", 11);
	}
	((char *)request->response.context)[11] = '\0';
	return(11);
}

/**
 * @brief Generate the response for a HEAD request from a file.
 * 
 * @param request Request that got us here.
 * @return Return unused.
 */
size_t ICACHE_FLASH_ATTR rest_gpio_head_handler(struct http_request *request)
{
	char str_size[16];
	size_t size;
	size_t ret = 0;
	
	//If the send buffer is over 200 bytes, this should never fill it.
	debug("REST GPIO HEAD handler.\n");
	switch(request->response.state)
	{
		case HTTP_STATE_STATUS:  ret = http_send_status_line(request->connection, request->response.status_code);
								 //Onwards
								 request->response.state = HTTP_STATE_HEADERS;
								 break;
		case HTTP_STATE_HEADERS: //Always send connections close and server info.
								 ret = http_send_header(request->connection, 
												  "Connection",
												  "close");
								 ret += http_send_header(request->connection,
												  "Server",
												  HTTP_SERVER_NAME);
								 //Get data size and create response.
								 if (current_gpio < 0)
								 {
									size = create_enabled_response(request);
								 }
								 else
								 {
									 size = create_pin_response(request);
								 }
								 os_sprintf(str_size, "%d", size);
								 //Send message length.
								 ret += http_send_header(request->connection, 
												  "Content-Length",
												  str_size);
								 ret += http_send_header(request->connection, "Content-Type", http_mime_types[MIME_JSON].type);	
								 //Send end of headers.
								 ret += http_send(request->connection, "\r\n", 2);
								 //Stop if only HEAD was requested.
								 if (request->type == HTTP_HEAD)
								 {
									 request->response.state = HTTP_STATE_ASSEMBLED;
								 }
								 else
								 {
									 request->response.state = HTTP_STATE_MESSAGE;
								 }
								 break;
	}
    return(ret);
}

/**
 * @brief Generate the HTML for configuring the gpio connection.
 * 
 * @param request Data for the request that got us here.
 * @return Size of the return massage.
 */
size_t ICACHE_FLASH_ATTR rest_gpio_get_handler(struct http_request *request)
{
	size_t msg_size = 0;
	char *uri = request->uri;
	size_t ret = 0;
	    
    debug("In GPIO GET REST handler (%s).\n", uri);
	
	//Don't duplicate, just call the head handler.
	if (request->response.state < HTTP_STATE_MESSAGE)
	{
		ret = rest_gpio_head_handler(request);
	}
	else
	{
		msg_size = os_strlen(request->response.context);
		debug(" Response: %s.\n", (char *)request->response.context);
		ret = http_send(request->connection, request->response.context, msg_size);
		request->response.state = HTTP_STATE_ASSEMBLED;
		
		debug(" Response size: %d.\n", msg_size);
		return(msg_size);
	}
	return(ret);
}

/**
 * @brief Set the gpio name for the WIFI connection.
 * 
 * @param request Data for the request that got us here.
 * @return The HTML.
 */
size_t ICACHE_FLASH_ATTR rest_gpio_put_handler(struct http_request *request)
{
	struct jsonparse_state json_state;
	char *uri = request->uri;
	int type;
	size_t ret = 0;
	unsigned int gpio_state;
	    
    debug("In GPIO PUT REST handler (%s).\n", uri);
    
	if (request->response.state == HTTP_STATE_STATUS)
	{
		//Handle trying to PUT to /rest/gpios
		if (current_gpio < 0)
		{
			ret = http_send_status_line(request->connection, 405);
		}
		else
		{
			ret = http_send_status_line(request->connection, 204);
		}
		request->response.state = HTTP_STATE_HEADERS;
	}
	else if (request->response.state == HTTP_STATE_HEADERS)
	{
		if (current_gpio < 0)
		{
			ret += http_send_header(request->connection, "Allow", "GET");
			ret += http_send_header(request->connection, "Content-length", "145");
		}
		//Send end of headers.
		ret += http_send(request->connection, "\r\n", 2);
		request->response.state = HTTP_STATE_MESSAGE;	
	}
	else if (request->response.state == HTTP_STATE_MESSAGE)
	{
		//Handle trying to PUT to /rest/gpios
		if (current_gpio < 0)
		{
			ret += http_send(request->connection, "<!DOCTYPE html><head><title>Method Not Allowed.</title></head><body><h1>405 Method Not Allowed.</h1><br />I won't PUT up with this.</body></html>", 145);
		}
		else
		{
			debug(" GPIO selected: %s.\n", request->message);
		
			jsonparse_setup(&json_state, request->message, os_strlen(request->message));
			while ((type = jsonparse_next(&json_state)) != 0)
			{
				if (type == JSON_TYPE_PAIR_NAME)
				{
					if (jsonparse_strcmp_value(&json_state, "state") == 0)
					{
						debug(" Found GPIO state in JSON.\n");
						jsonparse_next(&json_state);
						jsonparse_next(&json_state);
						gpio_state = jsonparse_get_value_as_int(&json_state);
						debug(" State: %d.\n", gpio_state);
						GPIO_OUTPUT_SET(current_gpio, gpio_state);
					}
				}
			}		
		}
		request->response.state = HTTP_STATE_ASSEMBLED;
		//We're not sending so we call this our selves.
		http_process_response(connection);	
	}
	return(ret);
}
/**
 * @brief Deallocate memory used for the HTML.
 * 
 * @param html A pointer to the mem to deallocate.
 */
void ICACHE_FLASH_ATTR rest_gpio_destroy(struct http_request *request)
{
	debug("Freeing GPIO REST handler data.\n");
	
	if (request->response.context)
	{
		db_free(request->response.context);
		request->response.context = NULL;
	}
}
