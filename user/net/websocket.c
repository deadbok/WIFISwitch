
/**
 * @file websocket.c
 *
 * @brief Websocket (RFC6455) connection handling.
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
#include "tools/sha1.h"
#include "tools/jsmn.h"
#include "tools/base64.h"
#include "tools/strxtra.h"
#include "slighttp/http.h"
#include "slighttp/http-tcp.h"
#include "slighttp/http-response.h"
#include "net/websocket.h"

/**
 * @brief Parse WebSocket frame header.
 *
 *     0                   1                   2                   3
 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-------+-+-------------+-------------------------------+
 *    |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 *    |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 *    |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 *    | |1|2|3|       |K|             |                               |
 *    +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 *    |     Extended payload length continued, if payload len == 127  |
 *    + - - - - - - - - - - - - - - - +-------------------------------+
 *    |                               |Masking-key, if MASK set to 1  |
 *    +-------------------------------+-------------------------------+
 *    | Masking-key (continued)       |          Payload Data         |
 *    +-------------------------------- - - - - - - - - - - - - - - - +
 *    :                     Payload Data continued ...                :
 *    + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 *    |                     Payload Data continued ...                |
 *    +---------------------------------------------------------------+
 *
 * @param data TCP data to parse.
 * @param frame Pointer to a frame to parse into.
 * @return Offset of payload.
 */
static size_t ws_parse_frame(char *data, struct ws_frame *frame)
{
	char *data_pos = data;
	
	if (*data_pos & 0x80)
	{
		frame->fin = true;
		debug(" Last frame.\n");
	}
	else
	{
		frame->fin = false;
	}	
	frame->rsv = (*data_pos & 0x70) >> 4;
	frame->opcode = (*data_pos++ & 0x0f);
	if (*data_pos & 0x80)
	{
		frame->mask = true;
		debug(" Data is masked.\n");
	}
	else
	{
		frame->mask = false;
	}
	frame->payload_len = (((unsigned char)(*data_pos++)) & 0x7f);
	debug(" Data length (7 bit) %" PRIu64 ".\n", frame->payload_len);
	if (frame->payload_len == 126)
	{
		frame->payload_len = ((uint16_t)(*data_pos));
		data_pos += 2;
		debug(" Data length (16 bit) %" PRIu64 ".\n", frame->payload_len);
	}
	if (frame->payload_len == 127)
	{
		frame->payload_len = ((uint64_t)(*data_pos));
		data_pos += 8;
		debug(" Data length (64 bit) %" PRIu64 ".\n", frame->payload_len);
	}
	os_memcpy(frame->masking_key, data_pos, 4);
	data_pos += 4;
	
	return(data_pos - data);
}

/**
 * @brief Called when data has been received.
 * 
 * Parse the WebSocket data, and call a protocol handler.
 * 
 * @param connection Pointer to the connection that received the data.
 */
static void ws_recv_cb(struct tcp_connection *connection)
{
	struct ws_frame frame;
	uint64_t i;
	size_t payload_offset;
	
	debug("WebSocket data received, %d bytes.\n", connection->callback_data.length);
	db_hexdump(connection->callback_data.data, connection->callback_data.length);
	payload_offset = ws_parse_frame(connection->callback_data.data, &frame);
	debug(" Payload at %d size %" PRIu64 ".\n", payload_offset, frame.payload_len);
	
	if (frame.opcode == WS_OPCODE_TEXT)
	{
		debug(" Text frame.\n");
		if (frame.mask)
		{
			debug(" Data is masked.\n");
			frame.data = db_malloc(frame.payload_len + 1, "frame.data ws_recv_cb");
			for (i = 0; i < frame.payload_len; i++)
			{
				frame.data[i] = connection->callback_data.data[i + payload_offset] ^ frame.masking_key[i % 4];
			}	
		
			frame.data[frame.payload_len] = '\0';
			debug(" Data %" PRIu64 " bytes: %s.\n", frame.payload_len, frame.data);
			
			/*
			 * wifiswitch protocol code.
			 */
			jsmn_parser parser;
			jsmntok_t tokens[10];
			int n_tokens;
			
			jsmn_init(&parser);
			n_tokens  = jsmn_parse(&parser, frame.data, frame.payload_len, tokens, 10);
			
			debug(" %d JSON tokens received.\n", n_tokens);
			if ( (n_tokens < 3) || tokens[0].type != JSMN_OBJECT)
			{
				warn("Could not parse JSON request.\n");
				return;
			}
			for (unsigned int i = 1; i < n_tokens; i++)
			{
				debug(" JSON token %d.\n", i);
				if (tokens[i].type == JSMN_STRING)
				{
					debug(" JSON token start with a string.\n");
					if (strncmp(frame.data + tokens[i].start, "type", 4) == 0)
					{
						i++;
						if (tokens[i].type == JSMN_STRING)
						{
							debug(" JSON string comes next.\n");
							
							char *type = frame.data + tokens[i].start;
							debug(" Request type %s.\n", type);							
						}
					}
				}
			}
		}
		ws_send_text("{ \"type\" : \"gpio\", \"gpios\" : [ 0, 1 ] }", connection);
	}
}

void ws_send_text(char *msg, struct tcp_connection *connection)
{
	struct ws_frame frame;
	
	if (msg)
	{
		frame.fin = true;
		frame.rsv = 0;
		frame.opcode = WS_OPCODE_TEXT;
		frame.mask = false;
		frame.payload_len = os_strlen(msg);
		frame.data = msg;
		
		ws_send(frame, connection);
	}
}
	
void ws_send(struct ws_frame frame, struct tcp_connection *connection)
{
	//14 is max header size
	size_t raw_frame_size = 14 + frame.payload_len;
	uint8_t *raw_frame;
	uint8_t *raw_frame_pos;
	
	raw_frame = db_malloc(raw_frame_size, "ws_send raw_frame");
	raw_frame_pos = raw_frame;
	*raw_frame_pos++ = ((((uint8_t)(frame.fin)) << 7) | (frame.rsv << 4) | (frame.opcode & 0x0f));
	//No Mask.
	if (frame.payload_len > 65536)
	{
		*raw_frame_pos++ = 127;
		(*((uint64_t *)(raw_frame_pos))) = frame.payload_len;
		raw_frame_pos += 8;
	}
	else if (frame.payload_len > 125)
	{
		*raw_frame_pos++ = 126;
		(*((uint16_t *)(raw_frame_pos))) = frame.payload_len;
		raw_frame_pos += 2;
	}
	else
	{
		*raw_frame_pos++ = (uint8_t)frame.payload_len;
	}
	//No Mask
	os_memcpy(raw_frame_pos, frame.data, frame.payload_len);
	raw_frame_pos += frame.payload_len;
	db_hexdump((char *)raw_frame, raw_frame_pos - raw_frame);
	tcp_send(connection, (char *)raw_frame, raw_frame_pos - raw_frame);
}

/**
 * @brief WebSocket data has been sent.
 *
 * @param Connection used to sent the data.
 */
static void ws_sent_cb(struct tcp_connection *connection)
{
	debug("WebSocket sent (%p).\n", connection);
}

void ws_register_sent_cb(struct tcp_connection *connection)
{
	connection->callbacks->sent_callback = ws_sent_cb;
	debug(" WebSocket sent callback %p set.\n", connection->callbacks->sent_callback);
}

void ws_register_recv_cb(struct tcp_connection *connection)
{
	connection->callbacks->recv_callback = ws_recv_cb;
	debug(" WebSocket receive callback %p set.\n", connection->callbacks->recv_callback);
}
