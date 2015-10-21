
/**
 * @file json-gen.c
 *
 * @brief Routines for generating JSON strings.
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
#include "c_types.h"
#include "eagle_soc.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_config.h"

/**
 * @brief Create a JSON array of strings.
 * 
 * @param values Array of strings.
 * @param entries Number of strings in array.
 * @return The JSON array.
 */
static char *json_create_string_array(char **values, size_t entries)
{
	size_t i;
	size_t total_length = 0;
	size_t *lengths = db_malloc(sizeof(size_t) * entries, "lengths json_create_string_array");
	char *ret, *ptr;
	
	//Get the length of the JSON array.
	//'['
	total_length++;
	for (i = 0; i < entries; i++)
	{
		//'\"'
		total_length++;
		lengths[i] = os_strlen(values[i]);
		total_length += lengths[i];
		//"\","
		total_length++;
		if (i != (entries -1))
		{
			total_length++;
		}
	}
	//']'
	total_length++;
	
	ret = (char *)db_malloc(total_length, "res json_create_string_array");
	ptr = ret;
	
	//Build the actual JSON array as a string.
	*ptr++ = '[';
	for (i = 0; i < entries; i++)
	{
		*ptr++ = '\"';
		os_memcpy(ptr, values[i], lengths[i]);
		ptr += lengths[i];
		*ptr++ = '\"';
		if (i != (entries -1))
		{
			*ptr++ = ',';
		}
	}
	*ptr++ = ']';
	*ptr = '\0';
	
	db_free(lengths);
	
	return(ret);
}
