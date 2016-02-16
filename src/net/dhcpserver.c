/**
 * @file dhcpserver.c
 *
 * @brief DHCP Server, RFC2131, RFC1553, and RFC951.
 * 
 * This as everything in here only does the bare minimum of what
 * could be expected. The implementation expect to be the only DHCP
 * server on the network. The assigned addresses, starts at the
 * IP after the server, and end at server IP + DHCPS_MAX_LEASES.
 * 
 * *Goto's has been harm-filtered.*
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
struct dhcps_lease
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
	 * @brief List entries.
	 */
	DL_LIST_CREATE(struct dhcps_lease);
};

/**
 * @brief Record of a client request.
 * 
 * **Pointers in this structure points into the data received.**
 */
struct dhcps_request
{
	/**
	 * @brief Requested IP address, IPADDR_ANY if none is requested.
	 */
	ip_addr_t *ip;
	/**
	 * @brief DHCP message type. Always set.
	 */
	uint8_t *type;
	/**
	 * @brief Requested hostname, NULL if none requested.
	 * 
	 * *First byte is the length.*
	 */
	uint8_t *hostname;
	/**
	 * @brief Client requested parameters.
	 * @see http://www.iana.org/assignments/bootp-dhcp-parameters/bootp-dhcp-parameters.xhtml
	 * 
	 * *First byte is the length.*
	 */
	uint8_t *parm;
	/**
	 * @brief Client-identifier, NULL if none was given.
	 * 
	 * *First byte is the length.*
	 */
	uint8_t *cid;	
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
 * Set either parameter to NULL if unused. If both parameters are NULL
 * return an unused lease, if any.
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
	
	//Get an unused address.
	if ((!hwaddr) && (!cid))
	{
		debug("First unused.\n");
		while (cur != NULL)
		{
			if (cur->expires)
			{
				debug("Found.\n");
				return(cur);
			}
			cur = cur->next;
		}
	}
		
	//Prefer client id, RFC2131 4.2 says so.
	if (cid)
	{
		debug(" Searching for client id.\n");
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
			cur = cur->next;
		}
	}
	
	//No client id, go by MAC.
	if (!cid)
	{
		debug(" Searching for MAC.\n");
		while (cur != NULL)
		{
			if (MACCMP(hwaddr, cur->hwaddr) && (cur->cid = NULL))
			{
				debug("Found.\n");
				return(cur);
			}
			cur = cur->next;
		}
	}
	debug("Not found.\n");
	return(NULL);
}

/**
 * @brief Get a lease for a MAC and/or client id.
 * 
 * Both `hwaddr`and `cid` cannot be NULL.
 * 
 * Search order:
 * 
 *  * Existing lease, both active and expired ones.
 *  * If all leases are taken, but some are unused, return the first one.
 *  * New lease containing only the client id and the MAC supplied.
 * 
 * The IP address of the lease is set to IP_ADDR_ANY, when first created.
 * 
 * @param hwaddr Pointer to the MAC address, or NULL if unused.
 * @param hwaddr Pointer to the client id, or NULL if unused.
 * @return Pointer to a lease, or NULL on failure.
 */
static struct dhcps_lease *get_lease(uint8_t *hwaddr, uint8_t *cid)
{
	struct dhcps_lease *lease = NULL;
	
	debug("Getting DHCP lease.\n");
	if ((!hwaddr) && (!cid))
	{
		warn(" No usable identifier for the lease.\n");
		return(NULL;
	}
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
	
	//Check for an existing lease for the client.
	lease = find_lease(hwaddr, cid);
	if (lease)
	{
		debug(" Returning an existing lease.\n");
		return(lease);
	}
	
	//We did not find an existing lease, check for an unused one.
	if (dhcps_ctx->n_leases >= DHCPS_MAX_LEASES)
	{
		lease = find_lease(NULL, NULL);
		if (lease)
		{
			debug(" Returning an unused lease.\n");
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
	lease->
	return(lease);
}

/**
 * @brief Add a lease.
 */
static bool add_lease(struct dhcps_lease *lease)
{
	struct dhcps_lease *leases = dhcps_ctx->leases;
	
	debug("Adding lease.\n");
	debug(" Client-identifier: ");
	DHCPS_DEBUG_ID(lease->cid);
	if (lease->hwaddr)
	{
		debug(" Hardware address: " MACSTR ".\n", MAC2STR(lease->hwaddr));
	}

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

	DL_LIST_ADD_END(lease, leases);
	dhcps_ctx->n_leases++;
	debug(" %d leases.\n", dhcps_ctx->n_leases);
	return(true);
}

static void answer_discover(struct dhcps_request *request, struct dhcp_msg *msg)
{
	struct netbuf *buf;
	struct dhcp_msg *reply;
	struct dhcps_lease *lease;
	 
	debug("Replying to discover.\n");
	
	//Get a new buffer for the reply.
	buf = netbuf_new();
	//Point the DHCP message structure to the actual memory.
	reply = (struct dhcp_msg *)netbuf_alloc(buf, DHCP_OPTIONS_LEN);
	bzero(reply, DHCP_OPTIONS_LEN);
	
	reply->op = DHCP_BOOTREPLY;
	reply->htype = DHCP_HTYPE_ETH;
	reply->hlen = 6;
	reply->xid = msg->xid;
	reply->flags = msg->flags;
	reply->giaddr = msg->giaddr;
//'options'  options  

	
/*	  o The client's current address as recorded in the client's current
        binding, ELSE

      o The client's previous address as recorded in the client's (now
        expired or released) binding, if that address is in the server's
        pool of available addresses and not already allocated, ELSE

      o The address requested in the 'Requested IP Address' option, if that
        address is valid and not already allocated, ELSE

      o A new address allocated from the server's pool of available
        addresses; the address is selected based on the subnet from which
        the message was received (if 'giaddr' is 0) or on the address of
        the relay agent that forwarded the message ('giaddr' when not 0).*/
     lease = get_lease(msg->hwaddr, request->cid);
     if (lease->ip != IP_ADDR_ANY)
     {
		 ip_addr_set(&reply->yiaddr, lease->ip;
	 }
	//'yiaddr'   IP address offered to client
	
	debug(" Sending reply.\n");
	/**
	 * @todo Read RFC2131 chapter 4.1, 50 times more.
	 * @todo Something about giaddr, and where to send this.
	 * @todo Something about the BROADCAST bit and something about unicast.
	 */
	netconn_sendto(dhcps_ctx->conn, buf, IP_ADDR_BROADCAST, DHCPS_PORT);
    netbuf_delete(buf);
}

/**
 * @brief Parse the option field of a DHCP message.
 */
static struct dhcps_request *parse_options(struct dhcp_msg *msg)
{
	struct dhcps_request *request = NULL;
	uint8_t *pos = msg->options;
	uint8_t len, opt;
	
	debug("Parsing options.\n");
	
	if (pos == NULL)
	{
		warn("No options.\n");
		goto error;
	}
	//Create record for the request.
	request = db_zalloc(sizeof(struct dhcps_request), 
						"request parse_options");
	
	/* As far as I can tell from the standard, the message type is
	 * required, but need not be the first option. */
	while ((*pos != DHCP_OPTION_END) && (pos < (msg->options + DHCP_OPTIONS_LEN)))
	{
		opt = *pos++;
		len = *pos++;

		if (opt == DHCP_OPTION_MESSAGE_TYPE)
		{
			request->type = pos;
			debug(" DHCP Message Type: %d.\n", *request->type);
			break;
		}
	}
	if (!request->type)
	{
		warn("No message type.\n");
		goto error;
	}
	
	pos = msg->options;
	while ((*pos != DHCP_OPTION_END) && 
		   (pos < (msg->options + DHCP_OPTIONS_LEN)))
	{
		opt = *pos++;
		len = *pos++;
		
		//See: http://www.iana.org/assignments/bootp-dhcp-parameters/bootp-dhcp-parameters.xhtml
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
					return(NULL);
				}
				//Set hostname, first byte is length.
				request->hostname = pos - 1;
				break;	
			case DHCP_OPTION_REQUESTED_IP:
				debug(" Requested IP Address,\n");
				if (len != 4)
				{
					warn("Unexpected length %d.\n", len);
					goto error;
				}
				/* ip_addr_t is a struct with a single member of type
				 * uint32_t (u32_t), a pointer to the struct, should
				 * point to the int as well. */
				request->ip = (ip_addr_t *)pos;
				break;
			case DHCP_OPTION_MESSAGE_TYPE:
				//Deja-vu.
				break;
			case DHCP_OPTION_PARAMETER_REQUEST_LIST:
				debug(" Parameter Request List.\n");
				//Include the length at index 0.
				request->parm = pos - 1;
				break;
			case DHCP_OPTION_CLIENT_ID:
				debug(" Client-identifier,\n");
				request->cid = pos - 1;
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
	return(request);
	
error:
	if (request)
	{
		db_free(request);
	}
	return(NULL);
}

/**
 * @brief Task to handle reciving.
 */
static void dhcps_task(void *pvParameters)
{
	while (1)
	{
		struct dhcps_request *request;
		struct dhcp_msg *msg;
		struct netbuf *buf;
		err_t ret;
		
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
		msg = (struct dhcp_msg *)buf->p;
	
		debug("UDP received on from " IPSTR ":%d.\n", IP2STR(netbuf_fromaddr(buf)), netbuf_fromport(buf));
		db_hexdump(buf->p->payload, buf->p->len);
		
		if (msg->op == DHCP_BOOTREQUEST)
		{
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
			request = parse_options(msg);
			if (!request)
			{
				warn("Could not parse request.\n");
				continue;
			}
			switch (*request->type)
			{
				case DHCP_DISCOVER:
					answer_discover(request, msg);
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
		}
		debug("Message from server.\n");
	}
	vTaskDelete(NULL);
}

bool dhcps_init(ip_addr_t *server_ip)
{
	uint8_t server_mac[NETIF_MAX_HWADDR_LEN];
	err_t ret;
	
	debug("Starting DHCP server.\n");
	if (dhcps_ctx)
	{
		error("Server running.\n");
		return(false);
	}
	dhcps_ctx = db_malloc(sizeof(struct dhcps_context), "dhcps_ctx dhcps_init");
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
	sdk_wifi_get_macaddr(SOFTAP_IF, server_mac);
	
	//Create a lease for the server.
	dhcps_ctx->leases = get_lease(server_mac, NULL);
	if (!dhcps_ctx->leases)
	{
		error(" Could not save server lease.\n");
		return(false);
	}
	memcpy(dhcps_ctx->leases->hwaddr, server_mac, NETIF_MAX_HWADDR_LEN);
	ip_addr_set(&dhcps_ctx->leases->ip, server_ip);
	dhcps_ctx->n_leases++;
	
	debug(" Creating tasks...\n");
    xTaskCreate(dhcps_task, (signed char *)"dhcps", 256, NULL, PRIO_DHCPS, dhcps_ctx->task);
	
	return(true);
}
