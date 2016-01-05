/** 
 * @file websocket.h
 *
 * @brief Websocket connection handling.
 * 
 * *Uses tcp_connection.user, as an array of WS_MAX_WAIT_FRAMES frames
 * waiting to be send.*
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
#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "net/tcp.h"

/**
 * @brief Maximum number of open websocket connections.
 */
#define WS_MAX_OPEN 10
/**
 * @brief Maximum number of registered WebSocket protocol handlers.
 */
#define WS_MAX_HANDLERS 10
/**
 * @brief Maximum size of the WebSocket  frame header.
 */
#define WS_MAX_HEADER_SIZE 14

/**
 * @brief Error return value for WebSocket functions.
 */
#define WS_ERROR -1

/**
 * @brief WebSocket frame opcode values.
 * 
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
	//Control frames.
	WS_OPCODE_CLOSE,
	WS_OPCODE_PING,
	WS_OPCODE_PONG,
	WS_OPCODE_RESB,
	WS_OPCODE_RESC,
	WS_OPCODE_RESD,
	WS_OPCODE_RESE,
	WS_OPCODE_RESF
};

/**
 * @brief Struct for keeping a WebSocket frame.
 */
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

/**
 * @brief Callback function definition for received frames.
 */
typedef signed long int (*ws_callback)(struct ws_frame *frame, struct net_connection *connection);
/**
 * @brief Type of an ID used to identify a registered WebSocket protocol handler.
 */
typedef unsigned char ws_handler_id_t;

/**
 * @brief Structure used for registering a WebSocket handler.
 */
struct ws_handler
{
	/**
	 * @brief Name of the protocol.
	 */
	char *protocol;
	/**
	 * @brief Callback on connection open.
	 */
	ws_callback open;
	/**
	 * @brief Callback on data received.
	 * 
	 */
	ws_callback receive;
	/**
	 * @brief Callback when data has been sent.
	 */
	ws_callback sent;
	/**
	 * Callback on connection close.
	 */
	ws_callback close;
	/**
	 * @brief Callback on reciving a ping frame.
	 */
	ws_callback ping;
	/**
	 * @brief Callback on reciving a pong frame.
	 */
	ws_callback pong;
};

/**
 * @brief Array of registered protocols.
 */
extern struct ws_handler ws_handlers[WS_MAX_HANDLERS];
/**
 * @brief Number of registered protocols.
 */
extern unsigned char ws_n_handlers;

/**
 * @brief WebSocket connection info.
 */
struct ws_connection
{
	/**
	 * @brief Handler for the connection.
	 */
	struct ws_handler *handler;
	/**
	 * @brief Is the connection closing.
	 * 
	 * Set by calling #ws_close.
	 * *If set when entering the sent callback, the TCP connection will
	 * be disconnected.*
	 */
	bool closing;
};
	

/**
 * @brief Initialise WebSocket server.
 */
extern void init_ws(void);
/**
 * @brief Register a WebSocket protocol handler.
 * 
 * @param handler Pointer to a ws_handler structure with handler data.
 * @return Handler ID on success, negative value on error.
 */
extern int ws_register_handler(struct ws_handler handler);
/**
 * @brief Unregister a webSocket protocol handler.
 */
extern bool ws_unregister_handler(ws_handler_id_t handler_id);
/**
 * @brief Find a handler for a protocol.
 * 
 * @param protocol Name of the protocol.
 */
extern int ws_find_handler(char *protocol);
/**
 * @brief Register WebSocket sent callback to a connection.
 * 
 * @param connection Connection to use.
 */
extern void ws_register_sent_cb(struct net_connection *connection);
/**
 * @brief Register WebSocket receive callback to a connection.
 * 
 * @param connection Connection to use.
 */
extern void ws_register_recv_cb(struct net_connection *connection);
/**
 * @brief Send a WebSocket frame.
 * 
 * @param frame WebSocket frame to send.
 * @param connection Connection to use.
 */
extern void ws_send(struct ws_frame frame, struct net_connection *connection);
/**
 * @brief Send a WebSocket text frame.
 * 
 * @param msg Text to send.
 * @param connection Connection to use.
 */
extern void ws_send_text(char *msg, struct net_connection *connection);
/**
 * @brief Close the WebSocket connection gracefully.
 */
extern void ws_close(struct net_connection *connection);
/**
 * @brief Free WebSocket connection.
 */
extern void ws_free(struct net_connection *connection);

#endif
