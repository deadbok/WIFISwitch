/**
 * @file dbffs.c
 * @brief Routines accessing a DBF file system in flash.
 * 
 * Header loader functions allocate DBFFS_MAX_HEADER_SIZE bytes of
 * memory so that any header data possible always fits.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "main.h"
#include "debug.h"
#include "fs/dbffs.h"
#include "fs/int_flash.h"

/**
 * @brief Load type and offset to next header.
 * 
 * @brief address Address to load the data from.
 * @return An array of 8 bytes with type and offset information or NULL.
 */
static uint32_t load_signature(unsigned int address)
{
	uint32_t ret;
	
	debug("Loading start of header at 0x%x.\n", address);
    if (!flash_aread(&ret, address, 4))
    {
        debug("Could not read DBFFS file header start at 0x%x.\n", address);
        return(0);
	}
	debug(" Signature 0x%x.\n", ret);
	return(ret);
}

/**
 * @brief Free memory used by the generic part of a header.
 * 
 * @param entry Pointer to the entry to free.
 */
static void free_generic_header(struct dbffs_generic_hdr *entry)
{
	if (entry)
	{
		if (entry->name)
		{
			debug("Freeing entry name.\n");
			db_free(entry->name);
		}
		debug("Freeing generic header.\n");
		db_free(entry);
	}
}

/**
 * @brief Load the generic part of a header, shared by all types.
 * 
 * @param address Address to load the data from.
 * @param header Pointer to memory for the header or NULL to allocate.
 * @return A pointer to the general header part.
 */
static struct dbffs_generic_hdr *load_generic_header(unsigned int address, void *header)
{
	struct dbffs_generic_hdr *ret = header;
	
	debug("Loading generic part of header at 0x%x.\n", address);
	if (!ret)
	{
		ret = db_malloc(DBFFS_MAX_HEADER_SIZE, "load_generic_header ret");
	}
	else
	{
		db_free(ret->name);
	}
	if (!ret)
	{
		error("Could not allocate memory for generic header.\n");
		return(NULL);
	}
    if (!flash_aread(ret, address, 9))
    {
        debug("Could not read DBFFS generic header start at 0x%x.\n", address);
        goto error;
	}
	ret->name = db_malloc(ret->name_len + 1, "load_generic_header ret->name"); 
	if (!flash_aread(ret->name, address + 9, ret->name_len))
    {
        debug("Could not read entry name at 0x%x.\n", address + 9);
        goto error;
    }
	ret->name[ret->name_len] = '\0';
	return(ret);

error:
    free_generic_header(ret);
    return(NULL);
}

void dbffs_free_file_header(struct dbffs_file_hdr *entry)
{
	if (entry)
	{
		if (entry->signature == DBFFS_FILE_SIG)
		{
			if (entry->name)
			{
				debug("Freeing entry name.\n");
				db_free(entry->name);
			}
			
			debug("Freeing generic header.\n");
			db_free(entry);
		}
		else
		{
			warn("Wrong header type trying to free file header 0x%x.\n", entry->signature);
		}
	}
}

/**
 * @brief Load a file header.
 * @brief address Address to load the data from.
 * @param header Pointer to memory for the header or NULL to allocate.
 * @return A pointer to the file header.
 */
static struct dbffs_file_hdr *load_file_header(unsigned int address, void *header)
{
	struct dbffs_file_hdr *ret;
	uint32_t offset = address;
	
	debug("Loading file header at 0x%x.\n", address);
	ret = (struct dbffs_file_hdr *)load_generic_header(address, header);
	offset += 9;
	offset += ret->name_len;
	if (!flash_aread(&ret->size, offset, sizeof(ret->size)))
    {
        debug("Could not read data size at 0x%x.\n", offset);
        dbffs_free_file_header(ret);
        return(NULL);
	}
	ret->data_addr = offset + sizeof(ret->size);
	
	return(ret);
}

/**
 * @brief Free memory used by a link header.
 * 
 * @param entry Pointer to the entry to free.
 */
static void free_link_header(struct dbffs_link_hdr *entry)
{
	if (entry)
	{
		if (entry->signature == DBFFS_LINK_SIG)
		{
			if (entry->name)
			{
				debug("Freeing entry name.\n");
				db_free(entry->name);
			}
			if (entry->target)
			{
				debug("Freeing target.\n");
				db_free(entry->target);
			}
			debug("Freeing generic header.\n");
			db_free(entry);
		}
		else
		{
			warn("Wrong header type trying to free link header 0x%x.\n",
				 entry->signature);
		}
	}
}

/**
 * @brief Load a link header.
 * @brief address Address to load the data from.
 * @param header Pointer to memory for the header or NULL to allocate.
 * @return A pointer to the link header.
 */
static struct dbffs_link_hdr *load_link_header(unsigned int address, void *header)
{
	struct dbffs_link_hdr *ret;
	uint32_t offset = address;
	
	debug("Loading link header at 0x%x.\n", address);
	ret = (struct dbffs_link_hdr *)load_generic_header(address, header);
	offset += 9;
	offset += ret->name_len;
	if (!flash_aread(&ret->target_len, offset, sizeof(ret->target_len)))
    {
        debug("Could not read target length at 0x%x.\n", offset);
        goto error;
	}
	offset += sizeof(ret->target_len);
	ret->target = db_malloc(ret->target_len + 1, "load_link_header ret->target"); 
	if (!flash_aread(ret->target, offset, ret->target_len))
    {
        debug("Could not read target name at 0x%x.\n", offset);
        goto error;
	}
	ret->target[ret->target_len] = '\0';
	return(ret);

error:
	free_link_header(ret);
	return(NULL);
}

struct dbffs_file_hdr *dbffs_find_file_header(char *path, void *header)
{
	struct dbffs_generic_hdr *gen_hdr;
	struct dbffs_file_hdr *file_hdr;
	struct dbffs_link_hdr *link_hdr;
	char *target;
						
	unsigned int hdr_off = 0;
	
	debug("Finding file header for %s.\n", path);
	if (!path)
	{
		error("Path is NULL.\n");
		return(NULL);
	}
	//Root
	gen_hdr = load_generic_header(hdr_off, header);
	while (gen_hdr)
	{
		debug("FS Address 0x%x.\n", hdr_off);
		debug("Header address %p.\n", gen_hdr);
	
		debug(" Signature 0x%x.\n", gen_hdr->signature);
		debug(" Offset to next entry 0x%x.\n", gen_hdr->next);
		debug(" Name length %d.\n", gen_hdr->name_len);
		debug(" Name %s.\n", gen_hdr->name);
		//Check current name against current path entry.
		if (strlen(path) == gen_hdr->name_len)
		{
			if (strncmp(gen_hdr->name, path, gen_hdr->name_len) == 0)
			{
				debug(" Entry name %s matches the path.\n",
					  gen_hdr->name);
				switch (gen_hdr->signature)
				{
					case DBFFS_FILE_SIG:
						file_hdr = load_file_header(hdr_off, gen_hdr);
						return(file_hdr);
					case DBFFS_LINK_SIG:
						link_hdr = load_link_header(hdr_off, gen_hdr);
						debug("Link target length %d.\n", link_hdr->target_len);
						debug("Link, target %s.\n", link_hdr->target);
						//Hijack the target name before it is overwritten.
						target = link_hdr->target;
						file_hdr = dbffs_find_file_header(target, link_hdr);
						db_free(target);
						return(file_hdr);
					default:
						warn("Unknown file entry signatures 0x%x.\n",
							 gen_hdr->signature);
				}
			}
		}
		if (gen_hdr->next)
		{
			hdr_off += gen_hdr->next;
			gen_hdr = load_generic_header(hdr_off, gen_hdr);
			if (!gen_hdr)
			{
				error("Could not load generic header part.\n");
				return(NULL);
			}
		}
		else
		{
			free_generic_header(gen_hdr);
			gen_hdr = NULL;
		}
	}
	debug("File not found.\n");
	return(NULL);
}

/**
 * @todo return error.
 */
void  init_dbffs(void)
{
    uint32_t signature;

    debug("Initialising DBBFS support.\n");
	fs_addr = cfg->fs_addr;
	debug(" File system at address 0x%x.\n", fs_addr + FLASH_OFFSET);
	signature = load_signature(0);
	if (signature == DBFFS_FS_SIG)
	{
		//Address of first header.
		fs_addr += sizeof(uint32_t);
	}
	else
	{
		error(" Could not find file system.\n");
		return;
	}

	debug(" Found file system at 0x%x.\n", fs_addr - sizeof(uint32_t)); 
}
