/** @file strxtra.c
 *
 * @brief Extra functions for srings.
 * 
 * Some extra utility functions for string handling.
 * 
 * Name    | Function
 * --------|-------------------------------------------------------------------
 * strchrs | Find a char from a set in a string.    
 * strlwr  | Convert string to lower case.
 * digits  | Find the size in characters of a string representation of an long.
 * strrpl  | Replace a string within another string.
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
#include <limits.h>
#include "c_types.h"
#include "osapi.h"

/**
 * @brief Find a character in a string.
 * 
 * Find any of the characters in @p chrs in @p str.
 * 
 * @param str The string to look in.
 * @param chrs A string consisting af all individual characters to look for.
 * @return A pointer to the first occurrence of any char in @p chrs or NULL.
 */
char *strchrs(char *str, char *chrs)
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
char *strlwr(char *str)
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
 * @brief Find the length of a long, when converted to a string.
 * 
 * @param n Count the number of tenth in this number.
 * @return The number of characters needed for the string version of
 *         the number.
 */
unsigned short digits(long n)
{
	unsigned short i = 1;
	unsigned long digit = 1;
	
	//Check sign, and adjust intitial values.
	if (n < 0)
	{
		//An extra chacarter for the sign.
		i++;
		//Make positive.
		n -= n;
	}
	
	//Keep increasing digits until we have enough.
	while((n > digit) && (digit < LONG_MAX))
	{
		digit *= 10;
		i++;
	}
	return(i);
}

/**
 * @brief Find the length of a float, when converted to a string.
 * 
 * @param n Count the number of digits in this float.
 * @param fractional_digits Number of digits after the decimal separator.
 * @return The number of characters needed for the string version of
 *         the number.
 */
unsigned short digits_f(float n, unsigned char fractional_digits)
{
	long int_part = n;
	
	//Add seperator and fractional characters.
	return(digits(int_part) + fractional_digits + 1);
}

/**
 * @brief Replace a string within another.
 * 
 * Replace a string within another.
 * 
 * @param src The string to operate on.
 * @param rpl The string to replace the old.
 * @param pos Position from where to start overwriting the old string.
 * @return Pointer to `src` or NULL on error.
 */
char *strrpl(char *src, char *rpl, size_t pos)
{
	size_t src_size, rpl_size;
	
	src_size = os_strlen(src);
	rpl_size = os_strlen(rpl);
	if ((!src_size) || (!rpl_size))
	{
		error("No string in either %p, %p, or both.\n", src, rpl);
		//There is no string.
		return(NULL);
	}
	if ((pos + rpl_size) > src_size)
	{
		error("Resulting string is to long %d, original %d.\n", pos + rpl_size, src_size);
		//The new string is to long.
		return(NULL);
	}	
	os_memcpy(src + pos, rpl, rpl_size);
	return(src);
}
