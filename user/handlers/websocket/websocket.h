/** 
 * @file websocket.h
 *
 * @brief Websocket handler.
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
#ifndef HTTP_WEBSOCKET_H
#define HTTP_WEBSOCKET_H

#include "slighttp/http-handler.h"

#define HTTP_WS_PROTOCOL "wifiswitch"
/**
 *    The handshake from the client looks as follows:
 *
 *       GET /chat HTTP/1.1
 *       Host: server.example.com
 *       Upgrade: websocket
 *       Connection: Upgrade
 *       Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
 *       Origin: http://example.com
 *       Sec-WebSocket-Protocol: chat, superchat
 *       Sec-WebSocket-Version: 13
 *
 *  The handshake from the server looks as follows:
 *
 *       HTTP/1.1 101 Switching Protocols
 *       Upgrade: websocket
 *       Connection: Upgrade
 *       Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
 *       Sec-WebSocket-Protocol: chat
 */
/**
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
 */
 
struct http_ws_frame
{
	bool fin;
	unsigned char rsv;
	uint8_t opcode:4;
	uint8_t mask:1;
	uint64_t payload_len:7;
	union
	{
		uint16_t ext_payload_len16;
		uint64_t ext_payload_len64;
	};
	uint32_t masking_key;
	char* payloadData;
};

extern signed int http_ws_handler(struct http_request *request);

#endif
