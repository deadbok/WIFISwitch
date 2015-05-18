/** 
 * @file int_flash.c
 *
 * @brief Interface between flash and memzip.
 * 
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@ace2>
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
#include "mem.h"
#include "tools/missing_dec.h"
#include "memzip.h"

//#size of 0x10000.bin
//sz = 285312
//adj = sz/0x4000*0x4000+0x4000
//print 'Write to {:02x}'.format(adj+0x10000)

//Flash: W25Q32BV

//Easy convert read to printable format
#define FLASH2STR(a) (a)[0], (a)[1], (a)[2], (a)[3]
#define FLASHSTR "%02x:%02x:%02x:%02x"

/**
 * @brief Address of the zip file.
 */
#define FS_ADDR 0x14000

MEMZIP_FILE_HDR *zip_header;

static unsigned int read_addr = 0;

/**
 * @brief Dump flash contents from an address until size bytes has been dumped.
 * 
 * Parameters:
 *  - unsigned int src_addr: Start address.
 *  - unsigned int size: Number of bytes to dump.
 */
void ICACHE_FLASH_ATTR flash_dump(unsigned int src_addr, size_t size)
{
    size_t i;
    unsigned int buf;
    
    for (i = 0; i < (size >> 2); i++)
    {
        spi_flash_read(src_addr + (i << 2), &buf, 4);
        
        os_printf("%x:", src_addr + ( i << 2));
        os_printf(FLASHSTR, FLASH2STR((char *)&buf));
        //os_printf("%c%c%c%c", FLASH2STR((char *)&buf));
        os_printf("\n");
    }
}

void ICACHE_FLASH_ATTR *read_flash(size_t size)
{
    unsigned int addr = FS_ADDR + read_addr;
    void *data;
    
    debug("Reading %d bytes of flash at 0x%x...", size, addr);
    
    data = (void *)os_malloc(size);
    
    if (spi_flash_read(addr, (uint32 *)data, size) == SPI_FLASH_RESULT_OK)
    {
        debug("OK.\n");
        read_addr += size;
        return(data);
    }
    debug("Failed!\n");
    return(NULL);
}

void ICACHE_FLASH_ATTR seek_flash(unsigned int addr)
{
    read_addr += addr;
}

void ICACHE_FLASH_ATTR abs_seek_flash(unsigned int addr)
{
    read_addr = addr;
}

MEMZIP_FILE_HDR ICACHE_FLASH_ATTR *load_header(unsigned int address)
{
    debug("Loading header at 0x%x.\n", address);
    
    abs_seek_flash(address);
    return(read_flash(sizeof(MEMZIP_FILE_HDR)));
}
