
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
#include "user_config.h"
#include "tools/sha1.h"
#include "tools/base64.h"
#include "tools/strxtra.h"
#include "slighttp/http.h"
//#include "slighttp/http-mime.h"
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
	//char digest[41];
	char *value;
	//char *digest_pos;
	unsigned char i;
	
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
		request->response.state = HTTP_STATE_MESSAGE;
		
		return(ret);
	}
		
	if (request->response.state == HTTP_STATE_MESSAGE)
	{
		debug("WebSocket handler state %d.\n", request->response.state);
		//ret = http_send(request->connection, "ok", 2);

		//We're done sending the message.
		request->response.state = HTTP_STATE_MESSAGE;
		debug("WebSocket handler leaving state %d.\n", request->response.state);
		return(ret);
	}
	    
	if (request->response.state == HTTP_STATE_DONE)
	{
		debug("WebSocket handler entering state %d.\n", request->response.state);

		if (request->response.context)
		{
			debug(" Freeing context data.\n");
			db_free(request->response.context);
			request->response.context = NULL;
		}
		return(RESPONSE_DONE_FINAL);
	}

	return(RESPONSE_DONE_NO_DEALLOC);
}

