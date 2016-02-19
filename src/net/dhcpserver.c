/**
 * @file dhcpserver.c
 *
 * @brief DHCP Server, see RFC2131, RFC1553, and RFC951.
 * 
 * This as everything in here only does the bare minimum of what
 * could be expected. 
 * 
 *  * The implementation expect to be the only DHCP
 *    server on the network.
 *  * The assigned addresses, starts at the IP after the server, and 
 *    end at server IP + DHCPS_MAX_LEASES.
 *  * The server only handles /24 networks.
 *  * The server largely ignores anything BOOTP.
 *  * The server has no notion of subnets.
 *  * Dumps a lease when released, client may get a different IP.
 *  * Address requests from the client are ignored.
 *  * Lease time requests from client are ignored.
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
//#include "lwip/udp.h"
#include <lwip/api.h>

#include "fwconf.h"
#include "debug.h"
#include "main.h"
#include "tools/dl_list.h"
#include "net/dhcpserver.h"

#define DHCP_CHADDR_LEN 16U
			
#define MACCMP(a, b)                                                   \
			(((a)[0] == (b)[0]) &&                                     \
			 ((a)[1] == (b)[1]) &&                                     \
			 ((a)[2] == (b)[2]) &&                                     \
			 ((a)[3] == (b)[3]) &&                                     \
			 ((a)[4] == (b)[4]) &&                                     \
			 ((a)[5] == (b)[5]))

/**
 * @brief Record of a client lease.
 */
struct dhcps_lease
{
	/**
	 * @brief Hostname of the client.
	 */
	char *hostname;
	/**
	 * @brief MAC of the client.
	 */
	uint8_t hwaddr[DHCP_CHADDR_LEN];
	/**
	 * @brief IP address.
	 */
	ip_addr_t *ip;
	/**
	 * @brief Client-identifier, NULL if none was given.
	 * 
	 * *First byte is the length.*
	 */
	uint8_t *cid;
	/**
	 * @brief Server time, when the lease has expired.
	 * 
	 * Zero if the lease is unused.
	 */
	uint32_t expires;
	/**
	 * @brief State of the lease.
	 * 
	 * DHCPOFFER, DHCPACK, DHCPNAK.
	 */
	uint8_t state;
	/**
	 * @brief List entries.
	 */
	DL_LIST_CREATE(struct dhcps_lease);
};

/**
 * @brief Record of the state of the server between calls.
 */
struct dhcps_context
{
	/**
	 * @brief List of DHCP leases.
	 * 
	 * Servers own lease is always the first.
	 */
	struct dhcps_lease *leases;
	/**
	 * @brief Number of DHCP leases.
	 */
	uint8_t n_leases;
	/**
	 * @brief The netconn connection.
	 */
	struct netconn *conn;
	/**
	 * @brief DHCP server task handle.
	 */
	xTaskHandle task;
};
/**
 * @brief Global state of the server between calls.
 */
static struct dhcps_context *dhcps_ctx = NULL;


#ifdef DEBUG
static void print_id(uint8_t *cid)
{
	if (cid)
	{
		for (uint8_t i = 1; i < *cid; i++)
		{
			debug("0x%02x", cid[i]);
			if (i < *cid)
			{
				debug(":");
			}
		}
	}
	else
	{
		debug("none");
	}
	debug("\n");
}

	#define DHCPS_DEBUG_ID(cid) print_id(cid)
#else
	#define DHCPS_DEBUG_ID(id)
#endif


/**
 * @brief Find a lease by it's MAC and/or client id.
 * 
 * Set either parameter to NULL if unused.
 */
static struct dhcps_lease *find_lease(uint8_t *hwaddr, uint8_t *cid)
{
	struct dhcps_lease *cur = dhcps_ctx->leases;
	
	debug("Looking for lease.\n");
	debug(" Client-identifier: ");
	DHCPS_DEBUG_ID(cid);
	if (hwaddr)
	{
		debug(" Hardware address: " MACSTR ".\n", MAC2STR(hwaddr));
	}
	
	if ((!hwaddr) && (!cid))
	{
		warn(" No usable identifier for the lease.\n");
		return(NULL);
	}
		
	//Prefer client id, RFC2131 4.2 says so.
	if (cid)
	{
		debug(" Searching for client id");
		while (cur != NULL)
		{
			//If client id's match.
			if (!memcmp(cur->cid, cid, cid[0] + 1))
			{
				if ((!hwaddr) && (!cur->hwaddr))
				{
					debug("Found, no MAC.\n");
					return(cur);
				}
				if (MACCMP(hwaddr, cur->hwaddr))
				{
					debug("Found with MAC.\n");
					return(cur);
				}
			}
			printf(".");
			cur = cur->next;
		}
		printf("Not found.\n");
	}
	
	cur = dhcps_ctx->leases;
	//No client id, go by MAC.
	if (!cid)
	{
		debug(" Searching for MAC");
		while (cur != NULL)
		{
			
			if ((MACCMP(hwaddr, cur->hwaddr)) && (cur->cid == NULL))
			{
				debug("Found.\n");
				return(cur);
			}
			printf(".");
			cur = cur->next;
		}
	}
	debug("Not found.\n");
	return(NULL);
}

/** 
 * @brief Sort leases by IP address.
 * 
 * Sort the list of leases according to IP address, to make it easier
 * to find an free address.
 * 
 * *This only works on /24 networks.*
 */
static void sort_leases(void)
{
	bool changed = true;
	struct dhcps_lease *temp;
	struct dhcps_lease *lease;
	
	debug("Sorting leases.\n");
	//Keep swapping leases until sorted.
	while (changed)
	{
		debug(" From the top.\n");
		lease = dhcps_ctx->leases;
		changed = false;
		//Swap items until the last lease.
		while (lease->next)
		{
			debug(" IP " IPSTR ".\n", IP2STR(lease->ip));
			//Swap positions of the current and the next lease if needed.
			if (ip4_addr4(lease->ip) > ip4_addr4(lease->next->ip))
			{
				debug(" Swapping " IPSTR " with " IPSTR ".\n", IP2STR(lease->ip), IP2STR(lease->next->ip));
				
				//Make previous lease point forward to the next lease.
				if (lease->prev)
				{
					lease->prev->next = lease->next;
				}
				else
				{
					lease->next->prev = NULL;
				}
				temp = lease->next;
				//Make next lease point back to the previous.
				temp->prev = lease->prev;
				//Make the item after the next point back to this lease.
				if (temp)
				{
					temp->next->prev = lease;
				}
				//Make this lease point forward at the next next lease.
				lease->next = temp->next;
				//Make next lease point forward to this.
				temp = lease;
				//Make this lease point back at the next.
				lease->prev = temp;
				
				changed = true;
			}
		}
	}
}							

/**
 * @brief Add a lease.
 */
static bool add_lease(struct dhcps_lease *lease)
{
	struct dhcps_lease *leases = dhcps_ctx->leases;
	//This is here because of a the DL macros, not working in this case.
	struct dhcps_lease *next;
	
	debug("Adding lease.\n");

	if (!dhcps_ctx->leases)
	{
		error("Server has no lease.\n");
		return(false);
	}	
	if (find_lease(lease->hwaddr, lease->cid))
	{
		warn("Lease exist.\n");
		return(false);
	}
	//Since items are sorted, just insert when the IP is no longer higher than the lease in the list.
	debug("%p %p.\n", leases->next, lease->next);
	while (leases->next)
	{
		if (ip4_addr4(leases->next->ip) > ip4_addr4(lease->ip))
		{
			debug(" Adding before " IPSTR ".\n", IP2STR(leases->next->ip));
			next = leases->next;
			break;
		}
		leases = leases->next;
	}
	debug("%p %p.\n", leases->next, lease->next);
	if (!leases->next)
	{
		debug(" Adding to end.\n");
		lease->next = NULL;
		next = NULL;
	}
	debug("%p %p.\n", next, lease->next);
	DL_LIST_INSERT(lease, leases, next);
	debug("%p %p.\n", next, lease->next);
	dhcps_ctx->n_leases++;
	debug(" Current leases:\n");
	leases = dhcps_ctx->leases;
	while (leases)
	{
		debug("  " IPSTR " next %p.\n", IP2STR(leases->ip), leases->next);
		leases = leases->next;
	}
	debug(" %d leases.\n", dhcps_ctx->n_leases);
	return(true);
}

/**
 * @brief Free a lease.
 */
static void free_lease(struct dhcps_lease *lease)
{
	debug("Freeing lease.\n");
	debug(" Client-identifier: ");
	DHCPS_DEBUG_ID(lease->cid);
	if (lease->hwaddr)
	{
		debug(" Hardware address: " MACSTR ".\n", MAC2STR(lease->hwaddr));
	}
	debug(" IP address: " IPSTR ".\n", IP2STR(lease->ip));

	//Remove from leases if it is in there.
	if (dhcps_ctx->leases)
	{
		if (find_lease(lease->hwaddr, lease->cid))
		{
			debug(" Removing from lease pool.\n");
			DL_LIST_UNLINK(lease, dhcps_ctx->leases);
			dhcps_ctx->n_leases--;
		}
	}
	debug(" %d leases.\n", dhcps_ctx->n_leases);
	
	sort_leases();
	if (lease->hostname)
	{
		db_free(lease->hostname);
	}
	if (lease->ip)
	{
		db_free(lease->ip);
	}
	
	if (lease->cid)
	{
		db_free(lease->cid);
	}
}

/**
 * @brief Get a lease for a MAC and/or client id.
 * 
 * Both `hwaddr`and `cid` cannot be NULL.
 * The IP address of the lease is set to IP_ADDR_ANY, when first created.
 * 
 * @param hwaddr Pointer to the MAC address, or NULL if unused.
 * @param cid Pointer to the client id, or NULL if unused.
 * @return Pointer to a lease, or NULL on failure.
 */
static struct dhcps_lease *get_lease(uint8_t *hwaddr, uint8_t *cid)
{
	struct dhcps_lease *lease = NULL;
	
	debug("Getting DHCP lease.\n");
	debug(" Client-identifier: ");
	DHCPS_DEBUG_ID(cid);
	if (hwaddr)
	{
		debug(" Hardware address: " MACSTR ".\n", MAC2STR(hwaddr));
	}
	if (dhcps_ctx->n_leases >= DHCPS_MAX_LEASES)
	{
		warn(" Address pool exhausted.\n");
		return(NULL);
	}
	
	if (dhcps_ctx->leases)
	{
		//Check for an existing lease for the client.
		lease = find_lease(hwaddr, cid);
		if (lease)
		{
			debug(" Returning an existing lease.\n");
			return(lease);
		}
	}
	
	debug("Creating new lease.\n");
	lease = db_zalloc(sizeof(struct dhcps_lease), "lease get_lease");
	if (hwaddr)
	{
		memcpy(lease->hwaddr, hwaddr, 6);
	}
	if (cid)
	{
		lease->cid = db_malloc(cid[0] + 1, "lease->cid get_lease");
		memcpy(lease->cid, cid, cid[0] + 1);
	}
	lease->ip = IP_ADDR_ANY;
	return(lease);
}

/**
 * @brief Helper function to get the next free IP.
 * 
 * *This only works for /24 networks.*
 * /
 * @return Pointer to IP, or NULL on error.
 */
static ip_addr_t *get_next_ip(void)
{
	struct dhcps_lease *leases = dhcps_ctx->leases;
	ip_addr_t *ret = NULL;
	
	debug("Getting next free IP address.\n");
	
	if (dhcps_ctx->n_leases >= DHCPS_MAX_LEASES)
	{
		warn(" Address pool exhausted.\n");
		return(NULL);
	}		

	//Since items are sorted, just find a gap and use the first IP.
	while (leases->next)
	{
		if (leases->ip->addr != (leases->ip->addr - 1))
		{
			break;
		}
		leases = leases->next;
	}

	ret = db_malloc(sizeof(ip_addr_t), "ret get_next_ip");
	ip4_addr_set_u32(ret, leases->ip->addr);
	ip4_addr4(ret)++;
	return(ret);
}
			
/**
 * @brief Add an option tag to the DHCP options.
 * 
 * *No boundary checking is done.*
 */
static uint8_t *options_add_tag(uint8_t *options, uint8_t tag)
{
	*options++ = tag;
	return(options);
}

/**
 * @brief Add single byte data to the DHCP options.
 *
 * *No boundary checking is done.*
 */
static uint8_t *options_add_byte(uint8_t *options, uint8_t data)
{
	*options++ = 1;
	*options++ = data;
	return(options + 1);
}

/**
 * @brief Add a multi-byte data to the DHCP options.
 *
 * *No boundary checking is done.*
 */
//static uint8_t *options_add_bytes(uint8_t *options, uint8_t len, uint8_t *data)
//{
	//*options++ = len;
	//if (len)
	//{
		//memcpy(options, data, len);
	//}
	//return(options + len);
//}

/**
 * @brief Find an option in the option field of a DHCP message.
 */
static uint8_t *find_option(uint8_t *options, uint8_t option)
{
	uint8_t *pos = options;
	uint8_t type, len;
	
	debug("Looking for option %d.", option);
	
	if (pos == NULL)
	{
		warn("No options.\n");
		return(NULL);;
	}
	
	while ((*pos != DHCP_OPTION_END) && 
		   (pos < (options + DHCP_OPTIONS_LEN)))
	{
		debug(".");
		type = *pos++;
		len = *pos++;
		if (type == option)
		{
			debug("Found.\n");
			return(pos - 2);
		}
		if (type == DHCP_OPTION_PAD)
		{
			pos++;
		}
		else
		{
			pos += len;
		}
	}
	debug("Not found.\n");
	return(NULL);
}
	
/**
 * @brief Send DHCPNAK.
 */
static void send_nak(struct dhcp_msg *msg)
{
	struct netbuf *buf;
	struct dhcp_msg *reply;
	struct ip_addr reply_ip;
	uint8_t *options = NULL;
	
	//Get a new buffer for the reply.
	buf = netbuf_new();
	//Point the DHCP message structure to the actual memory.
	reply = (struct dhcp_msg *)netbuf_alloc(buf, sizeof(struct dhcp_msg));
	bzero(reply, DHCP_OPTIONS_LEN);
	
	//Fill in the reply.
	reply->op = DHCP_BOOTREPLY;
	reply->htype = DHCP_HTYPE_ETH;
	reply->hlen = 6;
	reply->xid = msg->xid;
	reply->flags = msg->flags;
	reply->giaddr = msg->giaddr;
	reply->ciaddr = msg->ciaddr;
	
	//giaddr == 0: Broadcast any DHCPNAK message to 0cffffffff.
	if (!msg->giaddr.addr)
	{
		ip4_addr_set_u32(&reply_ip, IPADDR_BROADCAST);
	}
	else
	{
		ip_addr_set(&reply_ip, &msg->giaddr);
	}

	//Add options cookie.
	reply->cookie = ntohl(DHCP_MAGIC_COOKIE);
	//Add DHCP message type DHCPNAK.
	options = options_add_tag(reply->options, DHCP_OPTION_MESSAGE_TYPE);
	options = options_add_byte(options, DHCP_NAK);
	options = options_add_tag(options, DHCP_OPTION_END);
	
	//Send the message.
	netconn_sendto(dhcps_ctx->conn, buf, &reply_ip, DHCPS_PORT);
    netbuf_delete(buf);
}
			
/**
 * @brief Send a reply to a discover request.
 * 
 * @todo Read RFC2131 chapter 4.1, 50 times more.
 * @todo Something about the BROADCAST bit and something about unicast.
 */
static void answer_discover(struct dhcp_msg *msg)
{
	struct netbuf *buf;
	struct dhcp_msg *reply;
	struct dhcps_lease *lease;
	uint8_t *cid;
	 
	debug("Replying to discover.\n");
		
	if (msg->giaddr.addr != 0x0)
	{
		warn("I do not do subnets.\n");
		send_nak(msg);
		return;
	}
	
	//Get a new buffer for the reply.
	buf = netbuf_new();
	//Point the DHCP message structure to the actual memory.
	reply = (struct dhcp_msg *)netbuf_alloc(buf, sizeof(struct dhcp_msg));
	bzero(reply, DHCP_OPTIONS_LEN);
	
	//Fill in the reply.
	reply->op = DHCP_BOOTREPLY;
	reply->htype = DHCP_HTYPE_ETH;
	reply->hlen = 6;
	reply->xid = msg->xid;
	reply->flags = msg->flags;
	reply->giaddr = msg->giaddr;
	
	cid = find_option(msg->options, DHCP_OPTION_CLIENT_ID);
	if (cid)
	{
		//Get past the type.
		cid++;
	}
	lease = get_lease(msg->chaddr, cid);
	//Set lease state.
	lease->state = DHCP_OFFER;
	//If we got a lease with no IP.
	if (lease->ip == IP_ADDR_ANY)
	{
		lease->ip = get_next_ip();
		if (!lease->ip)
		{
			warn("Got no IP address.\n");
			free_lease(lease);
			send_nak(msg);
			goto free_netbuf;
		}
	}
	ip_addr_set(&reply->yiaddr, lease->ip);	
	
	if (!add_lease(lease))
	{
		warn("Could not add lease");
		free_lease(lease);
		send_nak(msg);
		goto free_netbuf;
	}
	
	//'options'  options  

	debug(" Sending reply.\n");
	db_hexdump(buf->p->payload, buf->p->len);
	netconn_sendto(dhcps_ctx->conn, buf, IP_ADDR_BROADCAST, DHCPS_PORT);
	
free_netbuf:
    netbuf_delete(buf);
}

/**
 * @brief Task to handle reciving.
 */	
static void dhcps_task(void *pvParameters)
{
	while (1)
	{
		struct dhcp_msg *msg;
		struct netbuf *buf;
		err_t ret;
		uint8_t *type;
		
		if (!dhcps_ctx->leases)
		{
			error("Server has no lease.\n");
			continue;
		}

        // Wait for data.
        ret = netconn_recv(dhcps_ctx->conn, &buf);
        if (ret != ERR_OK)
        {
			warn("Error receiving DHCP package.\n");
			continue;
		}
		
		/**
		 * @todo Do something to check and handle the size of the options.
		 */
		msg = (struct dhcp_msg *)buf->p->payload;
			
		debug("UDP received from " IPSTR ":%d.\n", IP2STR(netbuf_fromaddr(buf)), netbuf_fromport(buf));
		db_hexdump(buf->p->payload, buf->p->len);
		
		if (msg->op == DHCP_BOOTREQUEST)
		{
			struct dhcps_lease *leases = dhcps_ctx->leases;
			
			debug("Message from client.\n");
			if (msg->cookie != ntohl(DHCP_MAGIC_COOKIE))
			{
				warn("Bad cookie: 0x%08x.\n", msg->cookie);
				continue;
			}
			debug("Nice cookie.\n");
			if (msg->htype != DHCP_HTYPE_ETH)
			{
				warn("Unknown hardware type.\n");
				continue;
			}		
			type = find_option(msg->options, DHCP_OPTION_MESSAGE_TYPE);
			if (!type)
			{
				warn("Could find DHCP message type.\n");
				continue;
			}
			switch (type[2])
			{
				case DHCP_DISCOVER:
					answer_discover(msg);
					break;
				case DHCP_REQUEST:
					debug("DHCP request.\n");
					break;
				case DHCP_DECLINE:
					debug("DHCP decline.\n");
					break;
				case DHCP_RELEASE:
					debug("DHCP release.\n");
					break;
				case DHCP_INFORM:
					debug("DHCP inform.\n");
					break;
			}
			while (leases)
			{
				debug("  " IPSTR " next %p.\n", IP2STR(leases->ip), leases->next);
				leases = leases->next;
			}
			debug(" %d leases.\n", dhcps_ctx->n_leases);
		}
		else
		{
			debug("Ignoring message from server.\n");
		}
		netbuf_free(buf);		
	}
	vTaskDelete(NULL);
}

bool dhcps_init(ip_addr_t *server_ip)
{
	uint8_t server_mac[DHCP_CHADDR_LEN];
	err_t ret;
	
	debug("Starting DHCP server.\n");
	if (dhcps_ctx)
	{
		error("Server running.\n");
		return(false);
	}
	dhcps_ctx = db_zalloc(sizeof(struct dhcps_context), "dhcps_ctx dhcps_init");
	dhcps_ctx->conn = netconn_new(NETCONN_UDP);
	if (!dhcps_ctx->conn)
	{
		error("Could not create UDP connection.\n");
		return(false);
	}
	
	ret = netconn_bind(dhcps_ctx->conn, IP_ADDR_ANY, DHCPS_PORT);
	if (ret != ERR_OK)
	{
		error("Could not bind UDP connection to port %d: %d.\n", DHCPS_PORT, ret);
		return(false);
	}
	
	/* Uses the AP interface. Don't see any reason for a DHCP server on
	 * a client, that has got its own address from somewhere reasonable,
	 * but then again, I might be wrong. */
	bzero(server_mac, DHCP_CHADDR_LEN);
	sdk_wifi_get_macaddr(SOFTAP_IF, server_mac);
	
	//Create a lease for the server.
	dhcps_ctx->leases = get_lease(server_mac, NULL);
	if (!dhcps_ctx->leases)
	{
		error(" Could not save server lease.\n");
		return(false);
	}
	memcpy(dhcps_ctx->leases->hwaddr, server_mac, DHCP_CHADDR_LEN);
	dhcps_ctx->leases->ip = db_malloc(sizeof(ip_addr_t), "dhcps_ctx->leases->ip dhcps_init");
	ip_addr_set(dhcps_ctx->leases->ip, server_ip);
	dhcps_ctx->n_leases++;
	
	debug(" Creating tasks...\n");
    xTaskCreate(dhcps_task, (signed char *)"dhcps", 256, NULL, PRIO_DHCPS, dhcps_ctx->task);
	
	return(true);
}
