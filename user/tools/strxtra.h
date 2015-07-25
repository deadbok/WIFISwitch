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

extern char *strchrs(char *str, char *chrs);
extern char *strlwr(char *str);
extern unsigned short digits(long n);
extern unsigned short digits_f(float n, unsigned char fractional_digits);
extern char *strrpl(char *src, char *rpl, size_t pos);
extern char *itoa(long value, char *result, const unsigned char base);
extern char *ftoa(float value, char *result, unsigned char fractional_digits);

#endif //STRXTRA_H
