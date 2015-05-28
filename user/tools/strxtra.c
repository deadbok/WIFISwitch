/** @file strxtra.c
 *
 * @brief Extra functions for srings.
 * 
 * Some extra utility functions for string handling.
 * 
 * First Header  | Second Header
 * ------------- | -------------
 * Content Cell  | Content Cell 
 * Content Cell  | Content Cell 
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
#include <stdlib.h>
#include <ctype.h>
#include "c_types.h"


/**
 *  @brief Find a character in a string.
 * 
 * Find any of the characters in @p chrs in @p str.
 * 
 * @param str The string to look in.
 * @param chrs A string consisting af all individual characters to look for.
 * @return A pointer to the first occurrence of any char in @p chrs or NULL.
 */
char ICACHE_FLASH_ATTR *strchrs(char *str, char *chrs)
{
    unsigned int n_ch = 0;
    char *ptr = str;
    
    //If no character is found and we have not reached the end of the string.
    while ((*ptr != chrs[n_ch]) && (*ptr != '\0'))
    {
        //If we have tested all chars.
        if (!chrs[++n_ch])
        {
            //Start from scratch on the chars
            n_ch = 0;
            //Go to the next character in the string.
            ptr++;
        }
    }
    //If we did not find anything, return NULL.
    if (*ptr == '\0')
    {
        ptr = NULL;
    }
    return(ptr);
}

/**
 * @brief Convert every character of a string to lower case.
 * 
 * @param str Pointer to the string to work on.
 * @return Pointer to the string.
 */
char ICACHE_FLASH_ATTR *strlwr(char *str)
{
    char    *ptr = str;
    
    //Go through all caharacters.
    for (; *ptr != '\0'; ptr++)
    {
        //Skip anything but upper case characters.
        if ( (*ptr < 'A') || (*ptr > 'Z'))
        {
            continue;
        }
        //Convert to lower case.
        *ptr = tolower((unsigned char)*ptr);
    }
    return(str);
}

/**
 * @brief Count the number of digits in a number.
 */
int ICACHE_FLASH_ATTR digits(long n)
{
	int i = 1;
	
	while(n)
	{
		n /= 10;
		i++;
	}
	return(i);
}
