/**
 * @file debug.c
 *
 * @brief Debug helper functions.
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

#ifdef DEBUG
#include <ctype.h>

/**
 * @brief Dump some mem as hex in a nice way.
 * 
 * Lifted from: [http://grapsus.net/blog/post/Hexadecimal-dump-in-C](http://grapsus.net/blog/post/Hexadecimal-dump-in-C).
 * 
 * @param mem Pointer to the memory to dump.
 * @param len Length of memory block to dump.
 */
void db_hexdump(void *mem, unsigned int len)
{
	unsigned char cols = 8;
	unsigned int i, j;
	unsigned int ch;

	for(i = 0; i < len + ((len % cols) ? (cols - len % cols) : 0); i++)
	{
		/* print offset */
		if(i % cols == 0)
		{
			db_printf("0x%06x: ", i);
		}

		/* print hex data */
		if(i < len)
		{
			db_printf("%02x ", 0xFF & ((char*)mem)[i]);
		}
		else /* end of block, just aligning for ASCII dump */
		{
			db_printf("   ");
		}
		
		/* print ASCII dump */
		if(i % cols == (cols - 1))
		{
			for(j = i - (cols - 1); j <= i; j++)
			{
				ch = ((char*)mem)[j];
				if(j >= len) /* end of block, not really printing */
				{
					db_printf(" ");
				}
				else if(isprint(ch)) /* printable char */
				{
					db_printf("%c", (char)(0xFF & ch));        
				}
				else /* other char */
				{
					db_printf(".");
				}
			}
			db_printf("\n");
		}
	}
}

#endif //DEBUG

#ifdef DEBUG_MEM
#include "user_interface.h"
#include "c_types.h"
#include "osapi.h"
#include "mem.h"

/**
 * @brief Information for an allocated memory block.
 */
struct dbg_mem_alloc_info
{
	/**
	 * @brief Size of the allocated block.
	 */
	size_t size;
	/**
	 * @brief Pointer to a descriptive string.
	 */
	char *info;
	/**
	 * @brief Pointer to the memory block.
	 */
	void *ptr;
};

/**
 * @brief Number of allocated blocks.
 */
static int dbg_mem_n_alloc;
/**
 * @brief Array holding info on maximum #DBG_MEM_MAX_INFOS number of allocated blocks.
 */
static struct dbg_mem_alloc_info dbg_mem_alloc_infos[DBG_MEM_MAX_INFOS];

/**
 * @brief Generic alloc function for memory debugging.
 * 
 * @param size Number of bytes to allocate.
 * @param zero Set to true if the memory block should be filled with zeroes.
 * @param info A string with info on the allocated memory.
 * @return A pointer to the allocated memory.
 */
void *db_alloc(size_t size, bool zero, char *info)
{
	void *ptr;
	size_t free;

	os_printf("Allocating %d bytes.\n", size);	
	if (zero)
	{
		ptr = os_zalloc(size);
		free = system_get_free_heap_size();
		db_printf("Free heap (zalloc): %d.\n", free);
	}
	else
	{
		ptr = os_malloc(size);
		free = system_get_free_heap_size();
		db_printf("Free heap (malloc): %d.\n", free);
	}
	if (!ptr)
	{
		error("Could not reallocate memory.\n");
		return(NULL);
	}
	
	//If free heap is unreasonably high, something is probably wrong. 
	if (free > 100000)
	{
		warn("Memory management seems to be corrupted.\n");
	}

	if (dbg_mem_n_alloc < DBG_MEM_MAX_INFOS)
	{
		dbg_mem_alloc_infos[dbg_mem_n_alloc].size = size;
		dbg_mem_alloc_infos[dbg_mem_n_alloc].info = info;
		dbg_mem_alloc_infos[dbg_mem_n_alloc].ptr = ptr;
	}
	dbg_mem_n_alloc++;
	
	db_printf("Allocated %p size %d info: %s.\n", ptr, size, info);	

    db_printf("Allocs: %d.\n", dbg_mem_n_alloc);
    return(ptr);
}

/**
 * @brief Generic realloc function for memory debugging.
 * 
 * @param ptr Pointer to memory to copy to new allocation.
 * @param size Number of bytes to allocate.
 * @param info A string with info on the allocated memory.
 * @return A pointer to the allocated memory.
 */
void *db_realloc(void *ptr, size_t size, char *info)
{
	int i = 0;
	void *ret;
	size_t free;
	

	os_printf("Reallocating %d bytes from %p.\n", size, ptr);	
	ret = os_realloc(ptr, size);
	if (!ret)
	{
		error("Could not reallocate memory.\n");
		return(NULL);
	}
	free = system_get_free_heap_size();
	
	/* If free heap is unreasonably high, something is probably wrong.
	 * I have seen this happen after freeing a pointer twice.
	 */
	if (free > 100000)
	{
		warn("Memory management seems to be corrupted.\n");
	}
	
	//Find the old memory info.
	while(i < dbg_mem_n_alloc)
	{
		if (dbg_mem_alloc_infos[i].ptr)
		{
			if (dbg_mem_alloc_infos[i].ptr == ptr)
			{
				break;
			}
		}
		i++;
	}
	
	dbg_mem_alloc_infos[i].size = size;
	dbg_mem_alloc_infos[i].info = info;
	dbg_mem_alloc_infos[i].ptr = ret;
	
	db_printf("Reallocated %p size %d from %p, info: %s.\n", ret, size, ptr, info);	

    db_printf("Allocs: %d.\n", dbg_mem_n_alloc);
    return(ret);
}

/**
 * @brief Generic dealloc function for memory debugging.
 * 
 * @param ptr Pointer to the memory to deallocate.
 */
void db_dealloc(void *ptr)
{
	unsigned short i = 0;

	db_printf("Freeing %p.\n", ptr);
#ifdef DEBUG_MEM_LIST
	db_printf("Listing memory allocations:\n");
#endif
	while(i < dbg_mem_n_alloc)
	{
		if (dbg_mem_alloc_infos[i].ptr)
		{
			if (dbg_mem_alloc_infos[i].ptr == ptr)
			{
				db_printf(" [%p] size %d info: %s.\n",
						  dbg_mem_alloc_infos[i].ptr,
						  dbg_mem_alloc_infos[i].size,
						  dbg_mem_alloc_infos[i].info);

				//Move the last entry here.
				os_memcpy(dbg_mem_alloc_infos + i, dbg_mem_alloc_infos + dbg_mem_n_alloc - 1, sizeof(struct dbg_mem_alloc_info));
			}
#ifdef DEBUG_MEM_LIST
			else
			{
				db_printf("  %p size %d info: %s.\n",
						  dbg_mem_alloc_infos[i].ptr,
						  dbg_mem_alloc_infos[i].size,
						  dbg_mem_alloc_infos[i].info);
			}
#endif
		}
		i++;
	}
	os_free(ptr);
	dbg_mem_n_alloc--;
	db_printf("Free heap (free): %d.\n", system_get_free_heap_size());
    db_printf("Allocs: %d.\n", dbg_mem_n_alloc);
}

/**
 * @brief List data for a max. of the first 500 memory allocations.
 */
void db_mem_list(void)
{
	unsigned short i = 0;

	db_printf("Free heap (free): %d.\n", system_get_free_heap_size());
	db_printf("Allocs: %d.\n", dbg_mem_n_alloc);
	while(i < dbg_mem_n_alloc)
	{
		db_printf("  %p size %d info: %s.\n",
				  dbg_mem_alloc_infos[i].ptr,
				  dbg_mem_alloc_infos[i].size,
				  dbg_mem_alloc_infos[i].info);
		i++;
	}
}

#endif //DEBUG_MEM
