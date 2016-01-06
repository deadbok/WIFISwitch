
/**
 * @file wifiswitch.c
 *
 * @brief Websocket wifiswitch protocol code.
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
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include "user_config.h"
#include "tools/strxtra.h"
#include "tools/jsmn.h"
#include "tools/json-gen.h"
#include "net/websocket.h"
#include "handlers/websocket/wifiswitch.h"

#define WS_PR_WIFISWITCH "wifiswitch"

static ws_handler_id_t ws_wifiswitch_hid;

struct ws_handler ws_wifiswitch_handler = { WS_PR_WIFISWITCH, NULL, ws_wifiswitch_received, NULL, ws_wifiswitch_close, NULL, NULL};

bool ws_register_wifiswitch(void)
{
	debug("Registering wifiswitch WebSocket protocol handler.\n");
	ws_wifiswitch_hid = ws_register_handler(ws_wifiswitch_handler);
	if (ws_wifiswitch_hid < 0)
	{
		return(false);
	}
	return(true);
}

/**
 * @brief Create response to a gpio type request.
 * 
 * @return Pointer to JSON response.
 */
static char *ws_wifiswitch_gpio_response(void)
{
	char json_value[3];
	char *json_response = NULL;
	char *pair;
	char *json_gpio_en = NULL;
	
	debug("Creating JSON gpio response.\n");
	
	//Create type object.
	pair = json_create_pair("type", "gpio", true);
	json_response = json_add_to_object(json_response, pair);
	db_free(pair); 
	//Run through all GPIO's.
	for (unsigned int i = 0; i < WS_WIFISWITCH_GPIO_PINS; i++)
	{
		if (((WS_WIFISWITCH_GPIO_ENABLED >> i) & 1) == 1)
		{
			//Create an array of the enabled ones.
			debug(" GPIO%d is enabled.\n", i);
			if (!itoa(i, json_value, 10))
			{
				warn("Could not convert GPIO pin number to string.\n");
			}
			//Starts of with json_gpio_en = NULL, to create a new array.
			json_gpio_en = json_add_to_array(json_gpio_en, json_value);
		}
	}
	pair = json_create_pair("gpios", json_gpio_en, false);
	debug(" Add \"%s\".\n", pair);
	json_response = json_add_to_object(json_response, pair);
	db_free(json_gpio_en);
	db_free(pair);
	//Run through all GPIO's, again. Ended up deciding it was the nicest way.
	for (unsigned int i = 0; i < WS_WIFISWITCH_GPIO_PINS; i++)
	{
		if (((WS_WIFISWITCH_GPIO_ENABLED >> i) & 1) == 1)
		{
			//Create an array of the enabled ones.
			debug(" GPIO%d is enabled.\n", i);
			if (!itoa(i, json_value, 10))
			{
				warn("Could not convert GPIO pin number to string.\n");
			}
			//Create an array of objects with GPIO state of enabled pins.
			if (GPIO_INPUT_GET(i))
			{
				debug(" GPIO is on.\n");
				pair = json_create_pair(json_value, "1", false);
			}
			else
			{
				debug(" GPIO is off.\n");
				pair = json_create_pair(json_value, "0", false);
			}				
			json_response = json_add_to_object(json_response, pair);
			db_free(pair);
			
		}
	}
	
	return(json_response);
}

static void ws_wifiswitch_gpio_parse(char *request, jsmn_parser *parser, jsmntok_t *tokens, int n_tokens)
{
	int i;
	char gpio = -1;
	char *token;
	char *token_end;

	debug("Parsing wifiswitch gpio request.\n");	
	//Assumes the top level token is an object.
	for (i = 1; i < n_tokens; i++)
	{
		debug(" JSON token %d.\n", i);
		token = request + tokens[i].start;
		if (tokens[i].type == JSMN_STRING)
		{
			token_end = token;
			debug(" JSON token is a string \"%s\".\n", token);
			gpio = strtol(token, &token_end, 10);
			if (token == token_end)
			{
				debug(" Unexpected token.\n");
				gpio = -1;
			}
		}
		if (tokens[i].type == JSMN_PRIMITIVE)
		{
			debug(" JSON token is a primitive.\n");
			if (gpio > -1)
			{
				debug(" Have GPIO%d, setting state %c.\n", gpio, *token);
				if ((*token == '0') || (*token == '1'))
				{
					GPIO_OUTPUT_SET(gpio, *token - 48);
				}
				else
				{
					warn("Unsupported GPIO state %c.\n", *token);
				}
				gpio = -1;
				
			}
		}
	}
}

signed long int ws_wifiswitch_received(struct ws_frame *frame, struct net_connection *connection)
{
	uint32_t value;
	char *response = NULL;
	signed long int ret = 0;
	jsmn_parser parser;
	jsmntok_t tokens[10];
	int n_tokens;
	
	debug("Wifiswitch WebSocket data received.\n");
	//Set timeout.
	connection->timeout = WS_WIFISWITCH_TIMEOUT;
	db_hexdump(frame->data, frame->payload_len);
	if (frame->opcode != WS_OPCODE_TEXT)
	{
		error(" I only understand text data.\n");
		return(WS_ERROR);
	}
	
	jsmn_init(&parser);
	n_tokens  = jsmn_parse(&parser, frame->data, frame->payload_len, tokens, 10);
	
	debug(" %d JSON tokens received.\n", n_tokens);
	if ( (n_tokens < 3) || tokens[0].type != JSMN_OBJECT)
	{
		warn("Could not parse JSON request.\n");
		return(WS_ERROR);
	}
	//Assumes the top level token is an object.
	for (unsigned int i = 1; i < n_tokens; i++)
	{
		debug(" JSON token %d.\n", i);
		if (tokens[i].type == JSMN_STRING)
		{
			value = frame->data[tokens[i].start] << 24 |
					frame->data[tokens[i].start + 1] << 16 |
					frame->data[tokens[i].start + 2] << 8 |
					frame->data[tokens[i].start + 3];
			debug(" JSON token start with a string (0x%.4x).\n", value);
			//0x74797065 is ASCII values for "type".
			if (value == 0x74797065)
			{
				i++;
				if (tokens[i].type == JSMN_STRING)
				{
					debug(" JSON string comes next.\n");			
					/* The first 2 bytes of type is unique by using them as an
					 * 16 bit uint, we save a string comparison.
					 */
		 			value = frame->data[tokens[i].start] << 8 |
							frame->data[tokens[i].start + 1];
					switch (value)
					{
						case 0x6677:
							debug(" fw request.\n");
							break;
						case 0x6e65:
							debug(" network request.\n");
							break;
						case 0x7374:
							debug(" station request.\n");
							break;
						case 0x6170:
							debug(" ap request.\n");
							break;
						case 0x6770:
							debug(" gpio message.\n");
							//If there are only three tokens, the only valid request is one with only type.
							if (n_tokens > 3)
							{
								debug(" Full GPIO request.\n");
								ws_wifiswitch_gpio_parse(frame->data, &parser, tokens, n_tokens);
							}
							response = ws_wifiswitch_gpio_response();

							break;
						default:
							warn("Unknown wifiswitch request (0x%.2x).\n", value);
					}
				}
				else
				{
					warn("Unexpected JSON type in wifiswitch request.\n");
				}
			}
			else
			{
				debug("Not a type token, ignoring.\n");
			}
		}
	}
	if (response)
	{
		ret = os_strlen(response);
		ws_send_text(response, connection);
		db_free(response);
	}
	
	return(ret);
}

void ws_wifiswitch_send_gpio_status(void)
{
	struct net_connection *connection = tcp_get_connections();
	struct ws_connection *ws_conn;
	char *response = NULL;
	
	debug("Sending GPIO status to WebSocker clients.\n");
	response = ws_wifiswitch_gpio_response();
	while (connection)
	{
		//Find WebSocket connections.
		if (connection->type == NET_CT_WS)
		{
			ws_conn = connection->user;
			if (ws_conn)
			{
				if (ws_conn->handler)
				{
					if (strncmp(ws_conn->handler->protocol, WS_PR_WIFISWITCH, strlen(WS_PR_WIFISWITCH)) == 0);
					{
						debug(" Sending to %p.\n", connection);
						ws_send_text(response, connection);
					}
				}
			}
		}			
		connection = connection->next;
	}
	db_free(response);
}

signed long int ws_wifiswitch_close(struct ws_frame *frame, struct net_connection *connection)
{
	signed long int ret = 0;
	
	debug("Wifiswitch WebSocket close received.\n");
	
	return(ret);
}
