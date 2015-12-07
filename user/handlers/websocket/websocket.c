
/**
 * @file websocket.c
 *
 * @brief Websocket handler (RFC6455).
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
//#include "c_types.h"
//#include "eagle_soc.h"
//#include "osapi.h"
//#include "user_interface.h"
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
#include "handlers/websocket/websocket.h"

#define HTTP_WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/**
 * Enumeration to map the required request headers.
 */
enum http_ws_headers
{
	HTTP_WS_HEADER_HOST,
	HTTP_WS_HEADER_UPGRADE,
	HTTP_WS_HEADER_CONNECTION,
	HTTP_WS_HEADER_KEY,
	HTTP_WS_HEADER_ORIGIN,
	HTTP_WS_HEADER_PROTOCOL,
	HTTP_WS_HEADER_VERSION,
	HTTP_N_WS_HEADERS
};

/**
 * @brief Define the name of all required request headers.
 * 
 * *Must be in the same order as the enumeration above.*
 */
#define HTTP_WS_HEADERS { "host",                                      \
						  "upgrade",                                   \
						  "connection",                                \
						  "sec-websocket-key",                         \
						  "origin",                                    \
						  "sec-websocket-protocol",                    \
						  "sec-websocket-version" }

/**
 * @brief Generate accept header value from the client key.
 */
static char *websocket_gen_accept_value(char *key)
{
	struct sha1_context context;
	unsigned short bits;
	size_t length;
	char *key_guid;
	char *key_guid_cur;
	char *value;
	
	debug("Creating accept value from key %s.\n", key);
	
	key_guid = db_zalloc(os_strlen(key) + os_strlen(HTTP_WS_GUID) + 1, "websocket_gen_accept_value key_guid");
	os_strcpy(key_guid, key);
	os_strcat(key_guid, HTTP_WS_GUID);
  	
  	key_guid_cur = key_guid;
  	length = os_strlen(key_guid);
  	debug("Hashing %s.\n", key_guid_cur);
  	sha1_init(&context);

	while (length)
	{
		if (length > 63)
		{
			bits = 512;
		}
		else
		{
			bits = length << 3;
		}
		sha1_process((uint32_t *)key_guid_cur, bits, &context);
		key_guid_cur += bits >> 3;
		length -= bits >> 3;
	};
	sha1_final(&context);
	
	//Convert digest to hex string.
	/*digest_pos = digest;
	for (i = 0; i < 20; i++)
	{
		itoa(context.digest.b[i], digest_pos, 16);
		digest_pos += 2;
	}
	db_hexdump(digest, 41);
	digest[40] = '\0';
	debug(" Digest: %s\n", digest);*/

	//Base64 encode.
	value = db_malloc(BASE64_LENGTH(20) + 1, "websocket_gen_accept_value value");
	if (!base64_encode((char *)context.digest.b, 20, value, BASE64_LENGTH(20) + 1))
	{
		error(" Base64 encoding failed.\n");
		return(NULL);
	}

	db_free(key_guid);
	debug("Done.\n");
	return(value);
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
 * Parse the HTTP request, and send a meaningful answer.
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


void ws_sent_cb(struct tcp_connection *connection )
{
	debug("WebSocket sent (%p).\n", connection);
}
/**
 * @brief Handle the callback from the handshake.
 * 
 * Handle the first callback after the handshake is sent,
 * and replace the handler with the WebSocket one.
 */
void http_ws_sent_cb(struct tcp_connection *connection)
{
	debug("WebSocket handshake sent (%p).\n", connection);
	
	tcp_sent_cb(connection);
	
	connection->callbacks->sent_callback = ws_sent_cb;
	debug(" WebSocket sent callback %p set.\n", connection->callbacks->sent_callback);	
}
	

/**
 * @brief Generate the response for a HEAD request from a file.
 * 
 * @param request Request that got us here.
 * @return Return unused.
 */
signed int http_ws_handler(struct http_request *request)
{
	//Should have a host header, so this should never be NULL.
	struct http_header *header = request->headers;
	char *needed_headers[] = HTTP_WS_HEADERS;
	signed int ret = 0;
	unsigned char i;
	char *key = NULL;
	char *accept_value;
	bool upgrade = false;
	
	debug("WebSocket access to %s.\n", request->uri);
	
	if (request->response.state == HTTP_STATE_NONE)
	{
		debug(" Evaluating headers.\n");
		while (header)
		{
			for (i = 0; i < HTTP_N_WS_HEADERS; i++)
			{
				if (os_strcmp(header->name, needed_headers[i]) == 0)
				{
					switch (i)
					{
						case HTTP_WS_HEADER_HOST:
							debug("  Host header.\n");
							break;
						case HTTP_WS_HEADER_UPGRADE:
							debug("  Upgrade header.\n");
							break;
						case HTTP_WS_HEADER_CONNECTION:
							debug("  Connection header.\n");
							break;
						case HTTP_WS_HEADER_KEY:
							debug("  Key header.\n");
							key = header->value;
							break;
						case HTTP_WS_HEADER_ORIGIN:
							debug("  Origin header.\n");
							break;
						case HTTP_WS_HEADER_PROTOCOL:
							debug("  Protocol header.\n");
							break;
						case HTTP_WS_HEADER_VERSION:
							debug("  Version header.\n");
							if (os_strncmp(header->value, "13", 2) != 0)
							{
								warn(" Unsupported WebSocket version %s.\n", header->value);
								request->response.status_code = 426;
								upgrade = true;
							}
							break;
					}
					debug("   Value: %s.\n", header->value);
				}
			}
			header = header->next;
		}
		
		//Generate reponse accept value from key.
		accept_value = websocket_gen_accept_value(key);
		
		//If nothing bad has happened.
		if (request->response.status_code == 200)
		{
			//Switching protocols.
			request->response.status_code = 101;
		}
		//Send handshake status and headers.
		ret = http_send_status_line(request->connection, request->response.status_code);
		
		//Version support error.
		if (upgrade)
		{
			ret += http_send_header(request->connection,
									"Sec-WebSocket-Version", "13");
			return(RESPONSE_DONE_FINAL);
		}
		
		//Upgrade.
		ret += http_send_header(request->connection, "Upgrade",
								"websocket");
		ret += http_send_header(request->connection, "Connection",
								"Upgrade");
		//Key.						
		ret += http_send_header(request->connection, "Sec-WebSocket-Accept",
								accept_value);
		//Protocol.
		ret += http_send_header(request->connection, "Sec-WebSocket-Protocol",
								HTTP_WS_PROTOCOL);
		//Send end of headers.
		ret += http_send(request->connection, "\r\n", 2);
		
		//Set WebSocket TCP handler on the connection.
		request->connection->callbacks->recv_callback = ws_recv_cb;
		debug(" WebSocket receive callback %p set.\n", request->connection->callbacks->recv_callback);
		request->connection->callbacks->sent_callback = http_ws_sent_cb;
		debug(" WebSocket sent callback %p set.\n", request->connection->callbacks->sent_callback);
		
		request->response.state = HTTP_STATE_DONE;
		
		return(ret);
	}
		
	return(RESPONSE_DONE_NO_DEALLOC);
}
