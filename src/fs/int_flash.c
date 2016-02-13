/** 
 * @file int_flash.c
 *
 * @brief Interface between flash and dbffs.
 * 
 * @todo Use stdint.h types.
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
#include <stdbool.h>
#include "espressif/esp_common.h"
#include "debug.h"
#include "fs/int_flash.h"

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
	unsigned int id = sdk_spi_flash_get_id(); 
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
        sdk_spi_flash_read(src_addr + (i << 2), &buf, 4);
        
        printf("%x:", src_addr + ( i << 2));
        printf(FLASHSTR, FLASH2STR((char *)&buf));
        //os_printf("%c%c%c%c", FLASH2STR((char *)&buf));
        printf("\n");
    }
}

void flash_dump_mem(unsigned int src_addr, size_t size)
{
    size_t i;
    unsigned int *buf = (unsigned int *)(0x40200000 + src_addr);
    unsigned int data;
    
    for (i = 0; i < (size >> 2); i++)
    {
        printf("%x:", (unsigned int)buf);
        data = *((unsigned int *)((unsigned int)buf & -0x03));
        printf(FLASHSTR, FLASH2STR((char *)&data));
        printf("\n");
        buf++;
    }
}

size_t flash_memcpy_read(unsigned char *d, unsigned int s, size_t len)
{
	unsigned int i;
	unsigned int temp;
	unsigned int unaligned;
	unsigned char *p_src;

	p_src = (unsigned char *)(FLASH_OFFSET + s);
	debug("Copying %d bytes from %p to %p.\n", len, p_src, d);

	for (i = 0; i < len; i++)
	{
		//Get the unaligned part of the address.
		unaligned = (unsigned int)(p_src) & 0x03;
		
		//Aligned read.
		temp = *((unsigned int *)((unsigned int)(p_src) - unaligned));
		//debug(" %p (align %d), %p: 0x%x  - ", d, unaligned, p_src, temp);

		/* Calculate the offset of the unaligned byte. Shift it into
		 * place, and isolate it. */
		*d = (unsigned char)((temp >> (unaligned << 3)) & 0xFF);
		//debug(" 0x%x.\n", *d);
		
		d++;
		p_src++;
	}
	return(i);
}

bool flash_aread(const void *data, unsigned int read_addr, size_t size)
{
    unsigned int addr = fs_addr + read_addr;
    size_t ret;
	
	debug("Reading %d bytes from 0x%x to %p.\n", size, addr, data);
	ret = flash_memcpy_read((unsigned char *)data, addr, size);
	if (size == ret)
	{
		return(true);
	}
		
    return(false);
}
