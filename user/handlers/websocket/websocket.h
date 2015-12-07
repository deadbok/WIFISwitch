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

/**
 * @brief Name of the protocols to support seperated by space.
 */
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
 * Opcode:  4 bits
 *
 * Defines the interpretation of the "Payload data".  If an unknown
 * opcode is received, the receiving endpoint MUST _Fail the
 * WebSocket Connection_.  The following values are defined.
 *
 * * %x0 denotes a continuation frame
 * * %x1 denotes a text frame
 * * %x2 denotes a binary frame
 * * %x3-7 are reserved for further non-control frames
 * * %x8 denotes a connection close
 * * %x9 denotes a ping
 * * %xA denotes a pong
 * * %xB-F are reserved for further control frames
 */
enum ws_opcode
{
	WS_OPCODE_CONT,
	WS_OPCODE_TEXT,
	WS_OPCODE_BIN,
	WS_OPCODE_RES3,
	WS_OPCODE_RES4,
	WS_OPCODE_RES5,
	WS_OPCODE_RES6,
	WS_OPCODE_RES7,
	WS_OPCODE_CLOSE,
	WS_OPCODE_PING,
	WS_OPCODE_PONG,
	WS_OPCODE_RESB,
	WS_OPCODE_RESC,
	WS_OPCODE_RESD,
	WS_OPCODE_RESE,
	WS_OPCODE_RESF
};

struct ws_frame
{
	bool fin;
	uint8_t rsv;
	uint8_t opcode;
	bool mask;
	uint64_t payload_len;
	uint8_t masking_key[4];
	char *data;
};

extern signed int http_ws_handler(struct http_request *request);
extern void ws_send(struct ws_frame frame, struct tcp_connection *connection);
extern void ws_send_text(char *msg, struct tcp_connection *connection);

#endif
