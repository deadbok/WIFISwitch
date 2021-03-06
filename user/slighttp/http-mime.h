/** @file http-mime.h
 *
 * @brief MIME types.
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
#ifndef HTTP_MIME_H
#define HTTP_MIME_H

/**
 * @brief Structure to represent a MIME-type.
 */
struct http_mime_type
{
	char *ext;
	char *type;
};

enum http_mime_enum
{
	MIME_HTM,
	MIME_HTML,
	MIME_CSS,
	MIME_JS,
	MIME_JSON,
	MIME_TXT,
	MIME_JPG,
	MIME_JPEG,
	MIME_PNG,
	MIME_ICO,
	MIME_GZIP,
	HTTP_N_MIME_TYPES
};

extern struct http_mime_type http_mime_types[HTTP_N_MIME_TYPES];

extern char *http_mime_get_ext(char *filename);

#endif //HTTP_TCP_H
