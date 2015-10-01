/** 
 * @file common.c
 *
 * @brief Common routines.
 * 
 * *This tool is meant for small file systems used on embedded
 * systems, and keeps everything in memory while building the
 * image.*
 *
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@@ace2>
 * 
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
#include <errno.h> //errno
#include <stdio.h> //fprintf, perror
#include <stdlib.h> //abort
#include <string.h>
#include <unistd.h> //readlink
#include "common.h"

char *rpath(const char *path, char *buf)
{
	const char *path_pos = path;
	char *ret;
	char *ret_pos;
	
	if (path[0] != '/')
	{
		die("Path most be absolute.");
	}
	if (buf)
	{
		ret = buf;
	}
	else
	{
		if (!(ret = calloc(strlen(path), sizeof(char))))
		{
			die("Could not allocate memory for new path.");
		}
	}
	ret_pos = ret;
	
	while (*path_pos != '\0')
	{
		if (*path_pos == '.')
		{
			path_pos++;
			
			switch (*path_pos)
			{
				case '/':
					//Current dir
					break;
				case '.':
					//Up one dir.
					ret_pos -= 2;
					while (*ret_pos != '/')
					{
						ret_pos--;
						if (ret_pos < ret)
						{
							die("Outside file system root.");
						}
					}
					path_pos++;
					break;
				default:
					*ret_pos++ = '.';
					*ret_pos++ = *path_pos++;
			}
		}
		else
		{
			*ret_pos++ = *path_pos++;
		}
	}
	*ret_pos = '\0';
	return(ret);
}
	
void die(const char* message)
{
	if (errno > 0)
	{
		perror(message);
	}
	else
	{
		fprintf(stderr, "Error: %s\n", message);
	}
	abort();
}
