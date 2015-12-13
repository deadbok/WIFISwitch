
/**
 * @file websocket.c
 *
 * @brief Websocket (RFC6455) connection handling.
 * 
 * In true ESP8266 fashion, a sent callback must happen between
 * sending data.
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
//#include "tools/sha1.h"
//#include "tools/base64.h"
//#include "tools/strxtra.h"
//#include "slighttp/http.h"
//#include "slighttp/http-tcp.h"
//#include "slighttp/http-response.h"
#include "net/websocket.h"


struct ws_handler ws_handlers[WS_MAX_HANDLERS];
unsigned char ws_n_handlers = 0;

/**
 * @brief Register a WebSocket protocol handler.
 * 
 * @param handler Pointer to a ws_handler structure with handler data.
 * @return Handler ID on success, negative value on error.
 */
int ws_register_handler(struct ws_handler handler)
{
	unsigned char i;
	
	debug(" Registering WebSocket protocol handler \"%s\".", handler.protocol);
	
	//Find an empty entry.
	for (i = 0; !ws_handlers[i].protocol && (i < ws_n_handlers); i++);
	
	if (ws_handlers[i].protocol)
	{
		debug(" Adding handler with ID %d.\n", i);
		os_memcpy(ws_handlers + i, &handler, sizeof(struct ws_handler));
	}
	else
	{
		error(" No more handlers allowed.\n");
		return(-1);
	}
	return(i);
}

/**
 * @brief Unregister a webSocket protocol handler.
 */
bool ws_unregister_handler(ws_handler_id_t handler_id)
{
	unsigned char i;
	
	debug(" Removing WebSocket protocol handler with ID %d.", handler_id);
	if (handler_id > WS_MAX_HANDLERS)
	{
		error(" Handler ID is wrong.\n");
		return(false);
	}
	
	//Find the last entry after the handler that is to be removed.
	for (i = handler_id; ws_handlers[i].protocol && (i < ws_n_handlers); i++);
	
	if (ws_handlers[i].protocol)
	{
		debug(" Overwriting with last handler %d.\n", i);
		os_memcpy(ws_handlers + handler_id, ws_handlers + i, sizeof(struct ws_handler));
		os_memset(ws_handlers + i, 0, sizeof(struct ws_handler));
	}
	else
	{
		debug(" Zeroing handler data %d.\n", handler_id);
		os_memset(ws_handlers + handler_id, 0, sizeof(struct ws_handler));
	}
	return(true);
}

int ws_find_handler(char *protocol)
{
	int i;
	
	debug("Finding WebSocket handler for protocol \"%s\".\n", protocol);
	
	for (i = 0; i < ws_n_handlers; i++)
	{
		if (os_strcmp(ws_handlers[i].protocol, protocol) != 0)
		{
			debug(" Protocol ID %d.\n", i);
			return(i);
		}
	}
	warn(" Could not find a matching protocol handler.\n");
}

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


static void ws_unmask(char *data, struct ws_frame *frame)
{
	uint64_t i;
	
	for (i = 0; i < frame->payload_len; i++)
	{
		frame->data[i] = data[i] ^ frame->masking_key[i % 4];
	}	
}	

static void ws_handle_binary(struct ws_frame *frame, struct tcp_connection *connection)
{
	debug(" Binary frame.\n");
}

static void ws_handle_text(struct ws_frame *frame, struct tcp_connection *connection)
{
	debug(" Text frame.\n");
}

static void ws_handle_control(struct ws_frame *frame, struct tcp_connection *connection)
{
	debug(" Control frame.\n");
	switch(frame->opcode)
	{
		case WS_OPCODE_CLOSE:
			ws_handle_control(frame, connection);
			break;
		case WS_OPCODE_PING:
			ws_handle_control(frame, connection);
			break;
		case WS_OPCODE_PONG:
			ws_handle_control(frame, connection);
			break;
		default:
			debug(" Uknown frame type 0x%x.\n", frame->opcode);
	}
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
	size_t payload_offset;
	
	debug("WebSocket data received, %d bytes.\n", connection->callback_data.length);
	db_hexdump(connection->callback_data.data, connection->callback_data.length);
	payload_offset = ws_parse_frame(connection->callback_data.data, &frame);
	if (!frame.mask)
	{
		error(" WebSocket data not masked.\n");
		return;
	}
	debug(" Payload at %d size %" PRIu64 ".\n", payload_offset, frame.payload_len);

	if (frame.opcode < WS_OPCODE_RES7)
	{
		debug(" Data frame.\n");
		frame.data = db_malloc(frame.payload_len + 1, "frame.data ws_recv_cb");
		ws_unmask(connection->callback_data.data + payload_offset, &frame);
		frame.data[frame.payload_len] = '\0';
		debug(" Data %" PRIu64 " bytes.\n", frame.payload_len);

		switch(frame.opcode)
		{
			case WS_OPCODE_TEXT:
				ws_handle_text(&frame, connection);
				break;
			case WS_OPCODE_BIN:
				ws_handle_binary(&frame, connection);
				break;
			default:
				debug(" Unknown frame type 0x%x.\n", frame.opcode);
		}
	}
	else
	{
		ws_handle_control(&frame, connection);
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
