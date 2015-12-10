/** @file udp.h
 *
 * @brief DNS data structures for the ESP8266.
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
#ifndef DNS_H
#define DNS_H

/**
 * @brief DNS header.
 */
struct dns_header
{
	/**
	 * @brief ID set by the sender.
	 */
	unsigned short ID; //16 bits.
	/**
	 * @brief Query/Response flag, 0 on query.
	 */
	unsigned char QR : 1; //1 bit.
	/**
	 * @brief Query message type.
	 */
	unsigned char opcode : 4; //4 bit.
	/**
	 * @brief 1 if the responding server is authoritative for the zone.
	 */
	unsigned char AA : 1; //1 bit.
	/**
	 * @brief 1 if message is truncated.
	 */
	unsigned char TC : 1; //1 bit.
	/**
	 * @brief 1 if recursion is desired. (Query)
	 */
	unsigned char RD : 1; //1 bit.
	/**
	 * @brief 1 if recursion is available. (Response)
	 */
	unsigned char RA: 1; //1 bit.
	/**
	 * @brief Reserved bits set to 0.
	 */
	unsigned char Z : 3; //3 bits.
	/**
	 * @brief Response code.
	 */
	unsigned char RCode: 4; //4 bits
	/**
	 * @brief Question count.
	 */
	unsigned short QDCount; //16 bits
	/**
	 * @brief Answer count.
	 */
	unsigned short ANCount; //16 bits.
	/**
	 * @brief Authority record count.
	 */
	unsigned short NSCount; //16 bits.
	/**
	 * @brief Additional record count.
	 */
	unsigned short ARCount; //16 bits.
} __attribute__((packed)); //96 bits / 8 = 12 bytes.

#endif
