
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
#include <stdbool.h>
#include <inttypes.h>
#include "user_config.h"
#include "tools/jsmn.h"
#include "net/websocket.h"
#include "handlers/websocket/wifiswitch.h"

#define WS_PR_WIFISWITCH "wifiswitch"

static ws_handler_id_t ws_wifiswitch_hid;

struct ws_handler ws_wifiswitch_handler = { WS_PR_WIFISWITCH, NULL, ws_wifiswitch_received, NULL, NULL, NULL, NULL};

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

signed long int ws_wifiswitch_received(struct ws_frame *frame, struct tcp_connection *connection)
{
	uint32_t value;
	
	debug("Wifiswitch WebSocket data received.\n");
	if (frame->opcode != WS_OPCODE_TEXT)
	{
		error(" I only understand text data.\n");
		return(WS_ERROR);
	}
	/*
	 * wifiswitch protocol code.
	 */
	jsmn_parser parser;
	jsmntok_t tokens[10];
	int n_tokens;
	
	jsmn_init(&parser);
	n_tokens  = jsmn_parse(&parser, frame->data, frame->payload_len, tokens, 10);
	
	debug(" %d JSON tokens received.\n", n_tokens);
	if ( (n_tokens < 3) || tokens[0].type != JSMN_OBJECT)
	{
		warn("Could not parse JSON request.\n");
		return(WS_ERROR);
	}
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
							if (++i == n_tokens)
							{
								debug(" GPIO read request.\n");
							}
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
				warn("Type token expected.\n");
			}
		}
	}
	ws_send_text("{ \"type\" : \"gpio\", \"gpios\" : [ 0, 1 ] }", connection);
	return(os_strlen("{ \"type\" : \"gpio\", \"gpios\" : [ 0, 1 ] }"));
}
