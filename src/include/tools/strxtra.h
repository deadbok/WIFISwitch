/** @file strxtra.h
 *
 * @brief Extra functions for srings.
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
#ifndef STRXTRA_H
#define STRXTRA_H

/**
 * @brief Find a character in a string.
 * 
 * Find any of the characters in @p chrs in @p str.
 * 
 * @param str The string to look in.
 * @param chrs A string consisting af all individual characters to look for.
 * @return A pointer to the first occurrence of any char in @p chrs or NULL.
 */
extern char *strchrs(char *str, char *chrs);
/**
 * @brief Convert every character of a string to lower case.
 * 
 * @param str Pointer to the string to work on.
 * @return Pointer to the string.
 */
extern char *strlwr(char *str);
/**
 * @brief Find the length of a long, when converted to a string.
 * 
 * @param n Count the number of tenth in this number.
 * @return The number of characters needed for the string version of
 *         the number.
 */
extern unsigned short digits(long n);
/**
 * @brief Find the length of a float, when converted to a string.
 * 
 * @param n Count the number of digits in this float.
 * @param fractional_digits Number of digits after the decimal separator.
 * @return The number of characters needed for the string version of
 *         the number.
 */
extern unsigned short digits_f(float n, unsigned char fractional_digits);
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
extern char *strrpl(char *src, char *rpl, size_t pos);
/**
 * @brief Convert an integer to a string.
 * 
 * @param value The value to be converted.
 * @param result A pointer to where the string is written.
 * @param base The base used to convert the value.
 * @return A pointer to #result, or NULL on error.
 */ 
extern char *itoa(long value, char *result, const unsigned char base);
/**
 * @brief Convert a base 10 float to a string.
 * 
 * @param value The value to be converted.
 * @param result A pointer to where the string is written.
 * @param fractional_digits Digits after the point.
 * @return A pointer to #result, or NULL on error.
 */ 
extern char *ftoa(float value, char *result, unsigned char fractional_digits);

#endif //STRXTRA_H
