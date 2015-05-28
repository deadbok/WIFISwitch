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

#ifdef DEBUG_MEM

#include "user_interface.h"
#include "c_types.h"
#include "osapi.h"
#include "mem.h"



struct dbg_mem_alloc_info
{
	size_t size;
	char *info;
	void *ptr;
};

int dbg_mem_n_alloc;
struct dbg_mem_alloc_info dbg_mem_alloc_infos[DBG_MEM_MAX_INFOS];

void ICACHE_FLASH_ATTR *db_alloc(size_t size, bool zero, char *info)
{
	void *ptr;

	os_printf("Allocating size %d bytes.\n", size);	
	if (zero)
	{
		ptr = os_zalloc(size);
		os_printf("Free heap (malloc): %d.\n", system_get_free_heap_size());
	}
	else
	{
		ptr = os_malloc(size);
		os_printf("Free heap (zalloc): %d.\n", system_get_free_heap_size());
	}
	

	if (dbg_mem_n_alloc < DBG_MEM_MAX_INFOS)
	{
		dbg_mem_alloc_infos[dbg_mem_n_alloc].size = size;
		dbg_mem_alloc_infos[dbg_mem_n_alloc].info = info;
		dbg_mem_alloc_infos[dbg_mem_n_alloc].ptr = ptr;
	}
	dbg_mem_n_alloc++;
	
	os_printf("Allocated %p size %d info: %s.\n", ptr, size, info);	

    os_printf("Allocs: %d.\n", dbg_mem_n_alloc);
    return(ptr);
}

void ICACHE_FLASH_ATTR db_dealloc(void *ptr)
{
	unsigned short i = 0;
	
	os_printf("Freeing %p.\n", ptr);
	os_printf("Listing memory allocations:\n");
	while(i < dbg_mem_n_alloc)
	{
		if (dbg_mem_alloc_infos[i].ptr == ptr)
		{
			os_printf(" [%p] size %d info: %s.\n",
					  dbg_mem_alloc_infos[i].ptr,
					  dbg_mem_alloc_infos[i].size,
					  dbg_mem_alloc_infos[i].info);
			
			//Move the last entry here.
			os_memcpy(dbg_mem_alloc_infos + i, dbg_mem_alloc_infos + dbg_mem_n_alloc - 1, sizeof(struct dbg_mem_alloc_info));
		}
		else
		{
			os_printf("  %p size %d info: %s.\n",
					  dbg_mem_alloc_infos[i].ptr,
					  dbg_mem_alloc_infos[i].size,
					  dbg_mem_alloc_infos[i].info);
		}
		i++;
	}
	
	os_free(ptr);
	dbg_mem_n_alloc--;
	os_printf("Free heap (free): %d.\n", system_get_free_heap_size());
    os_printf("Allocs: %d.\n", dbg_mem_n_alloc);
}

#endif //DEBUG_MEM
