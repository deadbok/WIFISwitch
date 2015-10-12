/**
 * @file capport.c
 *
 * @brief Captive portal routines for the ESP8266.
 * 
 * Emit fake DNS records to always redirect request here. See RFC1035
 * and [The TCP/IP guide - DNS Name Servers and Name Resolution](http://www.tcpipguide.com/free/t_DNSNameServersandNameResolution.htm).
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
#include "missing_dec.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "ip_addr.h"
#include "mem.h"
#include "user_config.h"
#include "udp.h"
#include "dns.h"

static char *capture_domain = NULL;

/**
 * Convert an unsigned 16 bit from host to network byte order.
 *
 * @param value Value to convert.
 * @return Swapped value.
 */
static unsigned int swap16(unsigned short value)
{
	return(((value & 0xff) << 8) | ((value & 0xff00) >> 8));
}

/**
 * @brief Callback when UDP is received on port 53 (DNS)
 *
 * @param connection Connection that received the data.
 */
static void dns_recv(struct udp_connection *connection)
{
	char domain[255];
	unsigned char response[512];
	unsigned int offset = 2;
	unsigned int length = 0;
	unsigned int i = 0;
	bool query = false;
	struct dns_header *header;

	debug(" DNS UDP received on %p.\n", connection);

	query = connection->callback_data.data[offset] && 0x01;
	if (!query)
	{
		debug(" DNS is not a query.\n");
		return;
	}
	debug(" DNS query.\n Getting domain.\n");
	//Skip the rest of the header.
	offset = 12;   
	//Get length of first label in in the first question.
	length = connection->callback_data.data[offset];
	//Get the name in the first question.
	while ((length > 0) && (i < 255))
	{
		debug(".");
		offset++;
		os_memcpy(domain + i, connection->callback_data.data + offset, length);
		offset += length;
		i += length;
		length = connection->callback_data.data[offset];
		if (length)
		{
			//Add a dot if there is more data
			domain[i++] = '.';
		}
	}
	domain[i++] = '\0';
	debug("\n Query domain %s.\n", domain);
	
	if (os_strncmp(domain, capture_domain, os_strlen(capture_domain)) == 0)
	{
		debug("Answering.\n");
		
		//Copy header.
		debug(" Copying header to response, %d bytes.\n", sizeof(struct dns_header));
		os_memcpy(response, connection->callback_data.data, sizeof(struct dns_header));
		//Add header to offset.
		i = sizeof(struct dns_header);
		
		header = (struct dns_header *)response;
		header->QDCount = swap16(1);
		header->ANCount = swap16(1);
		header->NSCount = 0;
		header->ARCount = 0;
		
		//Response.
		header->QR = 1;
		//Authoritative answer.
		header->AA = 1;
		//Not truncated.
		header->TC = 0;
		//No recursion.
		header->RA = 0;
		//Reserved.
		header->Z = 0;
		//No error.
		header->RCode = 0;
		
		//Copy rest of query.
		debug(" Copying question to response.\n");
		length = connection->callback_data.data[i];
		response[i++] = length;
		while(length > 0)
		{
			debug("  %d bytes.\n", length);
			os_memcpy(response + i, connection->callback_data.data + i,
					  length);
			//Go to end of data.
			i += length;
			length = connection->callback_data.data[i];
			response[i++] = length;
		}
		//Copy the QType and QClass.
		debug("  4 bytes.\n");
		os_memcpy(response + i, connection->callback_data.data + i, 4);
		i += 4;
		debug(" Adding resource records at %p.\n", connection->callback_data.data + i);
		//Point to the name at position 12.
		response[i++] = 0xC0;
		response[i++] = 0x0C;
		//Set type to address.
		response[i++] = 0x00;
		response[i++] = 0x01;
		//Set class to “IN”.
		response[i++] = 0x00;
		response[i++] = 0x01;
		//Only use response for name resolution yhis one time.		
		response[i++] = 0x00;
		response[i++] = 0x01;
		response[i++] = 0x00;
		response[i++] = 0x01;
		//Length of data.
		response[i++] = 0x00;
		response[i++] = 0x04;
		//Data (IP address).
		response[i++] = 192;
		response[i++] = 168;
		response[i++] = 4;
		response[i++] = 1;
		debug(" Final size of response: %d bytes.\n", i);
		
		//Send the response.
		db_udp_send(connection, (char *)response, i);
	}
}

/**
 * @brief Callback when UDP has been sent on port 53 (DNS)
 *
 * @param connection Connection that sent the data.
 */
static void dns_sent(struct udp_connection *connection)
{
	debug(" DNS UDP sent on %p.\n", connection);
}

/**
 * @brief Initialise the captive portal.
 * 
 * @param domain Answers any DNS request that start with this.
 * @return True on success.
 */
bool init_captive_portal(char *domain)
{
	size_t domain_length = os_strlen(domain);
	db_printf("Starting captive portal for domain \"%s\".\n", domain);
	//Init udp, fails on recall, but does not matter.
	if (!init_udp())
	{
		warn(" Could not initialise captive portal.\n");
	}
	//Save domain name.
	capture_domain = db_malloc(domain_length + 1, "capture_domain init_captive_portal");
	memcpy(capture_domain, domain, domain_length);
	capture_domain[domain_length] = '\0';
	//Start listening on DNS port (53).
	if (!udp_listen(53, dns_recv, dns_sent))
	{
		error(" Could not create listening connection.\n");
		return(false);
	}
	return(true);
} 
