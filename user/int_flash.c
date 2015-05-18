/* wifi_connect.c
 *
 * Routines for connecting th ESP8266 to a WIFI network.
 *
 * - Load a WIFI configuration.
 * - Try to connect to the AP.
 * - If unsuccessful create an AP presenting a HTTP to configure the 
 *   connection values.
 * 
 * What works.
 * - Nothin'.
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
#include "tools/missing_dec.h"
//#include "spiffs.h"

//#size of 0x10000.bin
//sz = 285312
//adj = sz/0x4000*0x4000+0x4000
//print 'Write to {:02x}'.format(adj+0x10000)

//Flash: W25Q32BV

//Easy convert read to printable format
#define FLASH2STR(a) (a)[0], (a)[1], (a)[2], (a)[3]
#define FLASHSTR "%02x:%02x:%02x:%02x"

/**
 * @brief Dump flash contents from an address until size bytes has been dumped.
 * 
 * Parameters:
 *  - unsigned int src_addr: Start address.
 *  - unsigned int size: Number of bytes to dump.
 */
void ICACHE_FLASH_ATTR flash_dump(unsigned int src_addr, unsigned int size)
{
    unsigned int    i;
    unsigned int    buf;
    
    for (i = 0; i < (size >> 2); i++)
    {
        spi_flash_read(src_addr + (i << 2), &buf, 4);
        
        os_printf("%x:", src_addr + ( i << 2));
        os_printf(FLASHSTR, FLASH2STR((char *)&buf));
        //os_printf("%c%c%c%c", FLASH2STR((char *)&buf));
        os_printf("\n");
    }
}

/*
static s32_t api_spiffs_read(u32_t addr, u32_t size, u8_t *dst)
{
  flashmem_read(dst, addr, size);
  return SPIFFS_OK;
}

static s32_t api_spiffs_write(u32_t addr, u32_t size, u8_t *src)
{
  //debugf("api_spiffs_write");
  flashmem_write(src, addr, size);
  return SPIFFS_OK;
}

static s32_t api_spiffs_erase(u32_t addr, u32_t size)
{
  debug("api_spiffs_erase");
  u32_t sect_first = flashmem_get_sector_of_address(addr);
  u32_t sect_last = sect_first;
  while( sect_first <= sect_last )
    if( !flashmem_erase_sector( sect_first ++ ) )
      return SPIFFS_ERR_INTERNAL;
  return SPIFFS_OK;
} 
*/
