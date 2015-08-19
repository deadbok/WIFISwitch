/** @file http-mime.c
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
#include "http-mime.h"

/**
 * @brief Mapping file extensions to MIME-types.
 */
struct http_mime_type http_mime_types[HTTP_N_MIME_TYPES] =
{
	{"htm", "text/htm"}, //MIME_HTM
	{"html", "text/html"}, //MIME_HTML
	{"css", "text/css"}, //MIME_CSS
	{"js", "text/javascript"}, //MIME_JS
	{"json", "application/json"}, //MIME_JSON
	{"txt", "text/plain"}, //MIME_TXT
	{"jpg", "image/jpeg"}, //MIME_JPG
	{"jpeg", "image/jpeg"}, //MIME_JPEG
	{"png", "image/png"}, //MIME_PNG
	{"ico", "image/x-icon"} //MIME_ICO
};
