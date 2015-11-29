/** 
 * @file base64.c
 *
 * @brief Base64 encoding (RFC4648).
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
#include "user_config.h"
#include "base64.h"

static const char base64_enc_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
							         "abcdefghijklmnopqrstuvwxyz"
							         "0123456789+/";

bool base64_encode(char *str, size_t str_len, char *buf, size_t buf_size)
{
	char *str_pos = str;
	char *str_end = str + str_len ;
	char *buf_pos = buf;
	char *buf_end = buf + buf_size;
	char octets[3];
	unsigned char n_octets;
	
	debug("Base64 encoding %s length %d.\n", str, str_len);
	
	while (str_pos < str_end)
	{
		debug(" %d:", str_pos - str);
		//Get three bytes, with zero padding.
		n_octets = 0;
		for (unsigned char i = 0; i < 3; i++)
		{
			if (str_pos < str_end)
			{
				n_octets++;
				octets[i] = *str_pos++;
				debug(".");
			}
			else
			{
				octets[i] = 0;
				debug("0");
			}
		}
		//Check if there is room in the buffer.
		if (buf_end < (buf_pos + 4))
		{
			error("Not enough room in output buffer.\n");
			return(false);
		}

		//Look up 6-bit values in the map.
		//First 6-bits.
		*buf_pos++ = base64_enc_map[(octets[0] >> 2)];
		debug("1");
		//Second.
		*buf_pos++ = base64_enc_map[((octets[0] & 0x03) << 4) |
									((octets[1] & 0xf0) >> 4)];
		debug("2");
		//Third or padding.
		if (n_octets > 1)
		{
			*buf_pos++ = base64_enc_map[((octets[1] & 0x0f) << 2) |
										((octets[2] & 0xc0) >> 6)];
			debug("3");
		}
		else
		{
			*buf_pos++ = '=';
			debug("=");
		}
		//Fourth or padding.
		if (n_octets > 2)
		{
			*buf_pos++ = base64_enc_map[(octets[2] & 0x3f)];
			debug("4");
		}
		else
		{
			*buf_pos++ = '=';
			debug("=");
		}
	}

	//Check if there is room in the buffer.
	if (buf_end < buf_pos)
	{
		error("Not enough room in output buffer for zero terminator.\n");
		return(false);
	}
	*buf_pos = '\0';


	debug("\n Done.\n");
	debug("Result:\n");
	db_hexdump(buf, buf_pos - buf);

	return(true);
}
