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
#include "int_flash.h"

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

/**
 * @brief Dump flash contents from an address until size bytes has been dumped.
 * 
 * Uses the Espressif API.
 * 
 * @param src_addr Start address.
 * @param size Number of bytes to dump.
 */
void ICACHE_FLASH_ATTR flash_dump(unsigned int src_addr, size_t size)
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

/**
 * @brief Dump flash contents from an address until size bytes has been dumped.
 * 
 * Uses mapped access.
 * 
 * @param src_addr Start address.
 * @param size Number of bytes to dump.
 */
void ICACHE_FLASH_ATTR flash_dump_mem(unsigned int src_addr, size_t size)
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
 * @brief Do an 4 bit aligned copy.
 * 
 * @param d Destination memory.
 * @param s Source memory.
 * @param len Bytes to copy.
 * @return Bytes actually copied.
 */
static size_t ICACHE_FLASH_ATTR amemcpy(unsigned char *d, unsigned char *s, size_t len)
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

		*d = (temp >> (8 * unaligned));
		
		//debug(" 0x%x.\n", *d);
		
		d++;
		s++;
	}
	return(i);
}

/**
 * @brief Read data from an arbitrary position in the flash.
 * 
 * @param data Pointer to a buffer to place the data in.
 * @param read_addr Address to read from.
 * @param size Bytes to read.
 * @return True if everything wen well, false otherwise.
 */
bool ICACHE_FLASH_ATTR aflash_read(const void *data, unsigned int read_addr, size_t size)
{
    unsigned int addr = 0x40200000 + FS_ADDR + read_addr;
    size_t ret;
	
	ret = amemcpy((unsigned char *)data, (unsigned char *)addr, size);
	if (size == ret)
	{
		return(true);
	}
		
    return(false);
}

