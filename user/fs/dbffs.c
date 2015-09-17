/**
 * @file dbffs.c
 * @brief Routines accessing a DBF file system in flash.
 *
 */
#include <stdint.h>
#include "osapi.h"
#include "tools/missing_dec.h"
#include "user_config.h"
#include "int_flash.h"
#include "dbffs.h"

/**
 * @brief Load type and offset to next header.
 * 
 * @brief address Address to load the data from.
 * @return An array of 8 bytes with type and offset information or NULL.
 */
static uint8_t ICACHE_FLASH_ATTR *dbffs_load_header_start(unsigned int address)
{
	uint8_t *ret;
	
	debug("Loading start of header at 0x%x.\n", address);
	ret = db_malloc(8, "dbffs_load_header_start ret");

    if (!aflash_read(ret, address, 8))
    {
        debug("Could not read DBFFS file header start at 0x%x.\n", address);
        db_free(ret);
        return(NULL);
	}
	debug(" Signature 0x%x.\n", *((uint32_t *)(ret)));
	return(ret);
}

struct dbffs_file_hdr *dbffs_find_file_header(char *path)
{
	return(NULL);
}

/**
 * @brief Initialise the zip routines.
 */
void ICACHE_FLASH_ATTR init_dbffs(void)
{
    uint8_t *entry_hdr;

    debug("Initialising DBBFS support.\n");
	debug(" Searching for file system.\n");
	//Find the root directory.
	while (fs_addr < MAX_FS_ADDR)
	{
		debug(" Trying 0x%x.\n", fs_addr);
		entry_hdr = dbffs_load_header_start(0);
		if (*((uint32 *)(entry_hdr)) == DBFFS_DIR_SIG)
		{
			break;
		}
		fs_addr += 0x1000;
	}
	if (fs_addr == MAX_FS_ADDR)
	{
		error(" Could not find file system.\n");
		return;
	}
	debug(" Found file system at 0x%x.\n", fs_addr); 
}
