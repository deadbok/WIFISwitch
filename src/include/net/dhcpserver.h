/**
 * @file dhcpserver.h
 *
 * @brief DHCP Server.
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
#ifndef DHCPSERVER_H
#define DHCPSERVER_H
#include <stdbool.h>

/**
 * @brief Port that the DHCP server listens on.
 */
#define DHCPS_PORT 67


#if BYTE_ORDER == BIG_ENDIAN
/** 
 * @brief Convert an IP address given by the four byte-parts.
 * 
 * @todo Check that this is really not possible with lwIP.
 * @todo Move to net.h.
 * 
 * Could not find this in lwIP.
 */
#define TO_IP4_ADDR(a, b, c, d)                                        \
	((u32_t)((a) & 0xff) << 24) |                                      \
    ((u32_t)((b) & 0xff) << 16) |                                      \
    ((u32_t)((c) & 0xff) << 8)  |                                      \
    (u32_t)((d) & 0xff)
    
#else
/**
 * @brief Convert an IP address given by the four byte-parts.
 * 
 * @todo Check that this is really not possible with lwIP.
 * @todo Move to net.h.
 * 
 * Could not find this in lwIP.
 */
#define TO_IP4_ADDR(a, b, c, d)                                        \
	((u32_t)((d) & 0xff) << 24) |                                      \
	((u32_t)((c) & 0xff) << 16) |                                      \
	((u32_t)((b) & 0xff) << 8)  |                                      \
	(u32_t)((a) & 0xff)

#endif

/**
 * @brief Default lease time in seconds.
 */
#ifndef DHCPS_LEASE_TIME
#define DHCPS_LEASE_TIME 3600
#endif

/**
 * @brief Maximum number of leases.
 */
#ifndef DHCPS_MAX_LEASES
#define DHCPS_MAX_LEASES 10
#endif


extern bool dhcps_init(ip_addr_t *server_ip);

#endif //DHCPSERVER_H
