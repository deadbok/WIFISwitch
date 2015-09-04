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
#include "slighttp/http-handler.h"
#include "tools/strxtra.h"
#include "user_config.h"

#ifndef REST_GPIO_ENABLED
#warning "No GPIO's has been enabled for acces via the REST interface."
/**
 * @brief Bit mask of which GPIO pins the REST interface can control.
 */
#define REST_GPIO_ENABLED 0
#endif

//GPIO access.
extern bool rest_gpio_test(struct http_request *request);
extern void rest_gpio_header(struct http_request *request, char *header_line);
extern signed int rest_gpio_head_handler(struct http_request *request);
extern signed int rest_gpio_get_handler(struct http_request *request);
extern signed int rest_gpio_put_handler(struct http_request *request);
extern void rest_gpio_destroy(struct http_request *request);

/**
 * @brief Struct used to register the handler.
 */
struct http_response_handler http_rest_gpio_handler =
{
	rest_gpio_test,
	rest_gpio_header,
	{
		NULL,
		rest_gpio_get_handler,
		rest_gpio_head_handler,
		NULL,
		rest_gpio_put_handler,
		NULL,
		NULL,
		NULL
	}, 
	rest_gpio_destroy
};

/**
 * @brief context, that is stored with the request.
 */
struct rest_gpio_context
{
	/**
	 * @brief The response.
	 */
	char *response;
	/**
	 * @brief True if current response state is done.
	 */
	bool done;
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
		request->response.context = db_zalloc(sizeof(struct rest_gpio_context),
									   "request->response.context create_pin_response");
    }
    if (!ret)
    {
		debug("Rest handler GPIO will not handle request.\n"); 
	}
    return(ret);       
}


/**
 * @brief Handle headers.
 * 
 * @param request The request to handle.
 * @param header_line Header line to handle.
 */
void ICACHE_FLASH_ATTR rest_gpio_header(struct http_request *request, char *header_line)
{
	debug("HTTP REST GPIO header handler.\n");
}

/**
 * @brief Create a JSON array of integers.
 * 
 * @param values Pointer to an array of long ints.
 * @param entries Number of entries in the array.
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
 * @return Size of response.
 */
static signed int create_enabled_response(struct http_request *request)
{
	unsigned char n_gpios = 0;
	long gpios[16]  = { 0 };
	struct rest_gpio_context *context = request->response.context;
	
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
	context->response = json_create_int_array(gpios, n_gpios);
	
	return(os_strlen(context->response));
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
	struct rest_gpio_context *context = request->response.context;
	
	debug("Getting state of GPIO%d.\n", current_gpio);
	
	state = GPIO_INPUT_GET(current_gpio);
	
	debug(" GPIO state: %d.\n", state);
	
	context->response = db_malloc(sizeof(char) * 12,
										  "request->response.context->response create_pin_response");
	if (state)
	{
		os_memcpy(context->response, "{\"state\":1}", 11);
	}
	else
	{
		os_memcpy(context->response, "{\"state\":0}", 11);
	}
	context->response[11] = '\0';
	return(11);
}

/**
 * @brief Generate the response for a HEAD request from a file.
 * 
 * @param request Request that got us here.
 * @return Return unused.
 */
signed int ICACHE_FLASH_ATTR rest_gpio_head_handler(struct http_request *request)
{
	char str_size[16];
	size_t size;
	signed int ret = 0;
	struct rest_gpio_context *context = request->response.context;
	
	//If the send buffer is over 200 bytes, this should never fill it.
	debug("REST GPIO HEAD handler.\n");
	if (context->done)
	{
		debug(" State done.\n");
	}
	switch(request->response.state)
	{
		case HTTP_STATE_STATUS:  
			//Go on if we're done.
			if (context->done)
			{
				context->done = false;
				request->response.state++;
				return(0);
			}
			ret = http_send_status_line(request->connection, request->response.status_code);
			//Onwards
			context->done = true;
			break;
		case HTTP_STATE_HEADERS: 
			//Go on if we're done.
			if (context->done)
			{
				context->done = false;
				//Stop if only HEAD was requested.
				if (request->type == HTTP_HEAD)
				{
					request->response.state = HTTP_STATE_ASSEMBLED;
				}
				else
				{
					request->response.state = HTTP_STATE_MESSAGE;
				}				
				return(0);
			}
			//Always send connections close and server info.
			ret = http_send_header(request->connection, "Connection",
								   "close");
			ret += http_send_header(request->connection, "Server",
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
									"Content-Length", str_size);
			ret += http_send_header(request->connection, "Content-Type",
									http_mime_types[MIME_JSON].type);	
			//Send end of headers.
			ret += http_send(request->connection, "\r\n", 2);
			context->done = true;
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
signed int ICACHE_FLASH_ATTR rest_gpio_get_handler(struct http_request *request)
{
	size_t msg_size = 0;
	char *uri = request->uri;
	signed int ret = 0;
	struct rest_gpio_context *context = request->response.context;
	    
    debug("In GPIO GET REST handler (%s).\n", uri);
    if (context->done)
	{
		debug(" State done.\n");
	}
	//Don't duplicate, just call the head handler.
	if (request->response.state < HTTP_STATE_MESSAGE)
	{
		ret = rest_gpio_head_handler(request);
	}
	else
	{
		//Go on if we're done.
		if (context->done)
		{
			context->done = false;
			request->response.state++;
			return(0);
		}
		msg_size = os_strlen(context->response);
		debug(" Response: %s.\n", context->response);
		ret = http_send(request->connection, context->response, msg_size);
		request->response.state = HTTP_STATE_ASSEMBLED;
		
		debug(" Response size: %d.\n", ret);
		request->response.message_size = ret;
		context->done = true;
	}
	return(ret);
}

/**
 * @brief Set the gpio name for the WIFI connection.
 * 
 * @param request Data for the request that got us here.
 * @return Size of message.
 */
signed int ICACHE_FLASH_ATTR rest_gpio_put_handler(struct http_request *request)
{
	struct jsonparse_state json_state;
	char *uri = request->uri;
	int type;
	signed int ret = 0;
	unsigned int gpio_state;
	struct rest_gpio_context *context = request->response.context;
	    
    debug("In GPIO PUT REST handler (%s).\n", uri);
    if (context->done)
	{
		debug(" State done.\n");
	}
	if (request->response.state == HTTP_STATE_STATUS)
	{
		//Go on if we're done.
		if (context->done)
		{
			context->done = false;
			request->response.state++;
			return(0);
		}
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
		context->done = true;
	}
	else if (request->response.state == HTTP_STATE_HEADERS)
	{
		//Go on if we're done.
		if (context->done)
		{
			context->done = false;
			request->response.state++;
			return(0);
		}
		if (current_gpio < 0)
		{
			ret += http_send_header(request->connection, "Allow", "GET");
			ret += http_send_header(request->connection, "Content-length", "145");
		}
		else
		{
			ret = http_send_header(request->connection, "Connection", "close");
			ret += http_send_header(request->connection, "Server", HTTP_SERVER_NAME);
		}
		//Send end of headers.
		ret += http_send(request->connection, "\r\n", 2);
		context->done = true;	
	}
	else if (request->response.state == HTTP_STATE_MESSAGE)
	{
		//Go on if we're done.
		if (context->done)
		{
			context->done = false;
			request->response.state++;
			return(0);
		}
		//Handle trying to PUT to /rest/gpios
		if (current_gpio < 0)
		{
			ret += http_send(request->connection, "<!DOCTYPE html><head><title>Method Not Allowed.</title></head><body><h1>405 Method Not Allowed.</h1><br />I won't PUT up with this.</body></html>", 145);
			ret = 145;
			context->done = true;
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
			request->response.state++;
			ret = 0;
		}
		request->response.message_size = ret;
	}
	return(ret);
}

/**
 * @brief Deallocate memory used for the HTML.
 * 
 * @param request A pointer to the request with the context to deallocate.
 */
void ICACHE_FLASH_ATTR rest_gpio_destroy(struct http_request *request)
{
	struct rest_gpio_context *context = request->response.context;
	
	debug("Freeing GPIO REST handler data.\n");
	
	if (context)
	{
		if (context->response)
		{
			debug(" Freeing response.\n");
			db_free(context->response);
		}
		debug(" Freeing context.\n");
		db_free(context);
		request->response.context = NULL;
	}
}
