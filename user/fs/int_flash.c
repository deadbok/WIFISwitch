/** 
 * @file int_flash.c
 *
 * @brief Interface between flash and memzip.
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
//spi_flash.h should include this.
#include "c_types.h"
#include "spi_flash.h"
#include "user_interface.h"
#include "osapi.h"
#include "mem.h"
#include "user_config.h"
#include "tools/missing_dec.h"
#include "fs/int_flash.h"
#include "debug.h"

//#size of 0x10000.bin
//sz = 285312
//adj = sz/0x4000*0x4000+0x4000
//print 'Write to {:02x}'.format(adj+0x10000)

//Flash: W25Q32BV

/**
 * @brief Easy convert read bytes to printable format.
 */
#define FLASH2STR(a) (a)[0], (a)[1], (a)[2], (a)[3]
/**
 * @brief Format string for printing the read bytes.
 */
#define FLASHSTR "%02x:%02x:%02x:%02x"

size_t fs_addr = 0xa000;

size_t flash_size(void)
{ 
	unsigned int id = spi_flash_get_id(); 
	unsigned char mfg_id = id & 0xff;
	unsigned char size_id = (id >> 16) & 0xff; 
	
	debug("Flash ID 0x%x.\n", id);
	if ((mfg_id != 0xEF) && (mfg_id != 0xC8)) // 0xEF is WinBond and 0xC8 is GigaDevice;
	{
		return(0);
	}
	return(1 << size_id);
}

void flash_dump(unsigned int src_addr, size_t size)
{
    size_t i;
    unsigned int buf;
    
    for (i = 0; i < (size >> 2); i++)
    {
        spi_flash_read(src_addr + (i << 2), &buf, 4);
        
        db_printf("%x:", src_addr + ( i << 2));
        db_printf(FLASHSTR, FLASH2STR((char *)&buf));
        //os_printf("%c%c%c%c", FLASH2STR((char *)&buf));
        db_printf("\n");
    }
}

void flash_dump_mem(unsigned int src_addr, size_t size)
{
    size_t i;
    unsigned int *buf = (unsigned int *)(0x40200000 + src_addr);
    unsigned int data;
    
    for (i = 0; i < (size >> 2); i++)
    {
        db_printf("%x:", (unsigned int)buf);
        data = *((unsigned int *)((unsigned int)buf & -0x03));
        db_printf(FLASHSTR, FLASH2STR((char *)&data));
        db_printf("\n");
        buf++;
    }
}

/**
 * @brief Do a 4 bit aligned copy.
 * 
 * @param d Destination memory.
 * @param s Source memory.
 * @param len Bytes to copy.
 * @return Bytes actually copied.
 */
static size_t amemcpy(unsigned char *d, unsigned char *s, size_t len)
{
	unsigned int i;
	unsigned int temp;
	unsigned int unaligned;
	
	debug("Copying %d bytes from %p to %p.\n", len, s, d);

	for (i = 0; i < len; i++)
	{
		unaligned = (unsigned int)(s) & 0x03;
		
		//Aligned read.
		temp = *((unsigned int *)((unsigned int)(s) - unaligned));
		//debug(" %p (align %d), %p: 0x%x  - ", d, unaligned, s, temp);

		*d = (temp >> (unaligned << 3));
		
		//debug(" 0x%x.\n", *d);
		
		d++;
		s++;
	}
	return(i);
}

bool aflash_read(const void *data, unsigned int read_addr, size_t size)
{
    unsigned int addr = 0x40200000 + fs_addr + read_addr;
    size_t ret;
	
	debug("Reading %d bytes from 0x%x to %p.\n", size, addr, data);
	ret = amemcpy((unsigned char *)data, (unsigned char *)addr, size);
	if (size == ret)
	{
		return(true);
	}
		
    return(false);
}
