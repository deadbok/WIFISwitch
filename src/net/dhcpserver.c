/**
 * @file dhcpserver.c
 *
 * @brief DHCP Server, RFC2131, RFC1553, and RFC951.
 * 
 * This as everything in here only does the bare minimum of what
 * could be expected.
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
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "espressif/esp_common.h"
#include "sdk_internal.h"

/**
 * @brief Minimum required DHCP message options size, see RFC2131.
 */
#define DHCP_OPTIONS_LEN 312 

#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/udp.h"

#include "fwconf.h"
#include "debug.h"
#include "tools/dl_list.h"
#include "net/dhcpserver.h"

			
#define MACCMP(hwaddr1, hwaddr2)                                       \
			((hwaddr1[0] == hwaddr2[0]) &&                             \
			 (hwaddr1[1] == hwaddr2[1]) &&                             \
			 (hwaddr1[2] == hwaddr2[2]) &&                             \
			 (hwaddr1[3] == hwaddr2[3]) &&                             \
			 (hwaddr1[4] == hwaddr2[4]) &&                             \
			 (hwaddr1[5] == hwaddr2[5]))

/**
 * @brief Record of a client lease.
 */
struct dhcpserver_lease
{
	/**
	 * @brief Hostname of the client.
	 */
	char *hostname;
	/**
	 * @brief MAC of the client.
	 */
	uint8_t hwaddr[NETIF_MAX_HWADDR_LEN];
	/**
	 * @brief IP address.
	 */
	ip_addr_t ip;
	/**
	 * @brief List entries.
	 */
	DL_LIST_CREATE(struct dhcpserver_lease);
};

/**
 * @brief List of DHCP leases-
 */
static struct dhcpserver_lease *dhcpserver_leases = NULL;
/**
 * @brief number of DHCP leases.
 */
uint8_t n_leases = 0;

/**
 * @brief Find a lease by it's MAC.
 */
static struct dhcpserver_lease *find_by_hwaddr(uint8_t *hwaddr)
{
	struct dhcpserver_lease *cur = dhcpserver_leases;
	
	debug("Looking for lease with hwaddr: " MACSTR ".\n", 
		  MAC2STR(hwaddr));
	
	while (cur != NULL)
	{
		if (MACCMP(hwaddr, cur->hwaddr))
		{
			debug("Found.\n");
			return(cur);
		}
		cur = cur->next;
	}
	debug("Not found.\n");
	return(NULL);
}

/**
 * @brief Get a lease for a MAC.
 * 
 * If lease does not exist, a new one will be created.
 */
static struct dhcpserver_lease *get_lease(uint8_t *hwaddr)
{
	struct dhcpserver_lease *lease = NULL;
	
	debug("Getting DHCP lease for " MACSTR ".\n", MAC2STR(hwaddr));
	
	lease = find_by_hwaddr(hwaddr);
	if (lease)
	{
		debug(" Returning existing lease.\n");
		return(lease);
	}
	
	debug("Creating new lease.\n");
	lease = db_zalloc(sizeof(struct dhcpserver_lease), "lease get_lease");
	memcpy(lease->hwaddr, hwaddr, 6);
	
	return(lease);
}

/**
 * @brief Add a lease.
 */
static bool add_lease(struct dhcpserver_lease *lease)
{
	struct dhcpserver_lease *leases = dhcpserver_leases;
	
	debug("Adding lease for " MACSTR ".\n", MAC2STR(lease->hwaddr));
	
	if (find_by_hwaddr(lease->hwaddr))
	{
		warn("Lease exist.\n");
		return(false);
	}
	if (dhcpserver_leases == NULL)
	{
		dhcpserver_leases = lease;
		lease->prev = NULL;
		lease->next = NULL;
	}
	else
	{
		DL_LIST_ADD_END(lease, leases);
	}
	n_leases++;
	debug(" %d leases.\n", n_leases);
	return(true);
}

static void parse_options(struct dhcp_msg *msg)
{
	struct dhcpserver_lease *lease = NULL;
	ip_addr_t *request_ip = IP_ADDR_ANY;
	uint8_t *pos = msg->options;
	uint8_t len, opt;
	uint8_t *type = NULL;
	uint8_t *requests = NULL;
	
	debug("Parsing options.\n");
	
	if (pos == NULL)
	{
		warn("No options.\n");
		return;
	}
	lease = get_lease(msg->chaddr);
	
	/* As far as I can tell from the standard, the message type is
	 * required, but need not be the first option. */
	while ((*pos != DHCP_OPTION_END) && (pos < (msg->options + DHCP_OPTIONS_LEN)))
	{
		opt = *pos++;
		len = *pos++;

		if (opt == DHCP_OPTION_MESSAGE_TYPE)
		{
			type = pos;
			debug(" DHCP Message Type: %d.\n", *type);
			break;
		}
	}
	if (!type)
	{
		warn("No message type.\n");
		return;
	}
	
	pos = msg->options;
	while ((*pos != DHCP_OPTION_END) && 
		   (pos < (msg->options + DHCP_OPTIONS_LEN)))
	{
		opt = *pos++;
		len = *pos++;
		
		switch(opt)
		{
			case DHCP_OPTION_PAD:
				debug(" Padding.\n");
				break;
			case DHCP_OPTION_HOSTNAME:
				debug(" Host Name Option.\n");
				if (len < 1)
				{
					warn("Length cannot be zero.\n");
					return;
				}
				//Free old hostname.
				if (lease->hostname)
				{
					db_free(lease->hostname);
				}
				lease->hostname = db_malloc(len +1,
											"lease->hostname parse_option");
				strncpy(lease->hostname, (char *)pos, len);
				lease->hostname[len] = '\0';
				debug(" Hostname for " MACSTR " set to %s.\n",
					  MAC2STR(lease->hwaddr), lease->hostname);
				break;	
			case DHCP_OPTION_REQUESTED_IP:
				debug(" Requested IP Address: ");
				if (len != 4)
				{
					warn("Unexpected length %d.\n", len);
					return;
				}
				request_ip = db_malloc(sizeof(ip_addr_t),
									   "request_ip parse_option");
				IP4_ADDR(request_ip, *pos, *(pos + 1), *(pos + 2),
						 *(pos + 3));
				debug(IPSTR ".\n", IP2STR(&request_ip));
				break;
			case DHCP_OPTION_MESSAGE_TYPE:
				//Deja-vu.
				break;
			case DHCP_OPTION_PARAMETER_REQUEST_LIST:
				debug(" Parameter Request List: ");
				//Include the length at index 0.
				requests = pos - 1;
				while(len)
				{
					debug(" %d",
						  *(requests + requests[0] - len));
					len--;
				}
				len = requests[0];
				debug(".\n");
				break;
			default:
				debug(" Unknown option: %d.\n", opt);
				break;
		}
		pos += len;
	}
	if (*pos != DHCP_OPTION_END)
	{
		warn("Never found the end option.\n");
	}
}

//UDP receive callback
static void dhcpserver_rcv(void *arg, struct udp_pcb *pcb, struct pbuf *buf, ip_addr_t *addr, u16_t port)
{
	struct dhcp_msg *msg = buf->payload;
	
	debug("UDP received on from " IPSTR ":%d.\n", IP2STR(addr), port);
	db_hexdump(buf->payload, buf->len);
	
	if (msg->op == BOOTP_OP_BOOTREQUEST)
	{
		debug("Message from client.\n");
		if (msg->cookie != ntohl(DHCP_MAGIC_COOKIE))
		{
			debug("Bad cookie: 0x%08x.\n", msg->cookie);
			return;
		}
		debug("Nice cookie.\n");
		if (msg->htype != DHCP_HTYPE_ETH)
		{
			warn("Unknown hardware type.\n");
			return;
		}		
		parse_options(msg);
		
		return;
	}
	debug("Message from server.\n");
	pbuf_free(buf);
}

bool dhcpserver_init(ip_addr_t *server_ip)
{
	//protocol control block
	struct udp_pcb *pcb;
	uint8_t server_mac[NETIF_MAX_HWADDR_LEN];
	struct dhcpserver_lease *server_lease = NULL;
	
	debug("Starting DHCP server.\n");
	pcb = udp_new();
	if (!pcb)
	{
		error("Could not create UDP connection.\n");
		return(false);
	}
	
	int8_t ret;
	ret = udp_bind(pcb, IP_ADDR_ANY, DHCPSERVER_PORT);
	if (ret < 0)
	{
		error("Could not bind UDP connection to port 67: %d.\n", ret);
		return(false);
	}
	
	/* Uses the AP interface. Don't see any reason for a DHCP server on
	 * a client, that has got its own address from somewhere reasonable,
	 * but then again, I might be wrong. */
	sdk_wifi_get_macaddr(SOFTAP_IF, server_mac);
	
	//Create a lease for the server.
	server_lease = get_lease(server_mac);
	if (!server_lease)
	{
		error(" Could not save server lease.\n");
		return(false);
	}
	memcpy(server_lease->hwaddr, server_mac, NETIF_MAX_HWADDR_LEN);
	ip_addr_set(&server_lease->ip, server_ip);
	add_lease(server_lease);
	
	udp_recv(pcb, dhcpserver_rcv, NULL);
	return(true);
}
