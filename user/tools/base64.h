/** 
 * @file base64.h
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
#ifndef BASE64_H
#define BASE64_H

#define BASE64_LENGTH(s) ((((s) + 2) * 4) / 3)

/**
 * Input length has to be a multiply of three when processing continuous
 * blocks, to avoid padding between them.
 */
extern bool base64_encode(char *str, size_t str_len, char *buf, size_t buf_size);

#endif
