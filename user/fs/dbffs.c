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
static uint32_t ICACHE_FLASH_ATTR load_signature(unsigned int address)
{
	uint32_t ret;
	
	debug("Loading start of header at 0x%x.\n", address);
    if (!aflash_read(&ret, address, 4))
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
static void ICACHE_FLASH_ATTR free_generic_header(struct dbffs_generic_hdr *entry)
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
 * @brief address Address to load the data from.
 * @return A pointer to the general header part.
 */
static struct dbffs_generic_hdr ICACHE_FLASH_ATTR *load_generic_header(unsigned int address)
{
	struct dbffs_generic_hdr *ret;
	
	debug("Loading generic part of header at 0x%x.\n", address);
	ret = db_malloc(sizeof(struct dbffs_generic_hdr), "load_generic_header ret");
	if (!ret)
	{
		error("Could not allocate memory for generic header.\n");
		return(NULL);
	}
    if (!aflash_read(ret, address, 9))
    {
        debug("Could not read DBFFS file header start at 0x%x.\n", address);
        free_generic_header(ret);
        return(NULL);
	}
	ret->name = db_malloc(ret->name_len + 1, "load_generic_header ret->name"); 
	if (!aflash_read(ret->name, address + 9, ret->name_len))
    {
        debug("Could not read entry name at 0x%x.\n", address + 9);
        free_generic_header(ret);
        return(NULL);
	}
	ret->name[ret->name_len] = '\0';
	return(ret);
}

/**
 * @brief Free memory used by a file header.
 * 
 * @param entry Pointer to the entry to free.
 */
void ICACHE_FLASH_ATTR dbffs_free_file_header(struct dbffs_file_hdr *entry)
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
 * @return A pointer to the file header.
 */
static struct dbffs_file_hdr ICACHE_FLASH_ATTR *load_file_header(unsigned int address)
{
	struct dbffs_file_hdr *ret;
	uint32_t offset = address;
	
	debug("Loading file header at 0x%x.\n", address);
	ret = db_malloc(sizeof(struct dbffs_file_hdr), "load_file_header ret");
	if (!ret)
	{
		error("Could not allocate memory for file header.\n");
		return(NULL);
	}
    if (!aflash_read(ret, offset, 9))
    {
        debug("Could not read DBFFS file header start at 0x%x.\n", address);
        dbffs_free_file_header(ret);
        return(NULL);
	}
	offset += 9;
	ret->name = db_malloc(ret->name_len + 1, "load_file_header ret->name"); 
	if (!aflash_read(ret->name, offset, ret->name_len))
    {
        debug("Could not read entry name at 0x%x.\n", offset);
        dbffs_free_file_header(ret);
        return(NULL);
	}
	offset += ret->name_len;
	ret->name[ret->name_len] = '\0';
	if (!aflash_read(&ret->size, offset, sizeof(ret->size)))
    {
        debug("Could not read data size at 0x%x.\n", offset);
        dbffs_free_file_header(ret);
        return(NULL);
	}
	ret->data_addr = offset + sizeof(ret->size);
	
	return(ret);
}

/**
 * @brief Free memory used by a directory header.
 * 
 * @param entry Pointer to the entry to free.
 */
static void ICACHE_FLASH_ATTR free_dir_header(struct dbffs_dir_hdr *entry)
{
	if (entry)
	{
		if (entry->signature == DBFFS_DIR_SIG)
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
			warn("Wrong header type trying to free dir header 0x%x.\n",
				 entry->signature);
		}
	}
}

/**
 * @brief Load a directory header.
 * @brief address Address to load the data from.
 * @return A pointer to the directory header.
 */
static struct dbffs_dir_hdr ICACHE_FLASH_ATTR *load_dir_header(unsigned int address)
{
	struct dbffs_dir_hdr *ret = NULL;
	uint32_t offset = address;
	
	debug("Loading directory header at 0x%x.\n", address);
	/*ret = db_malloc(sizeof(struct dbffs_dir_hdr), "load_dir_header ret");
	if (!ret)
	{
		error("Could not allocate memory for directory header.\n");
		return(NULL);
	}*/
	//aflash_read(ret, offset, 4);
	//aflash_read(ret, offset + 4, 4);
	//aflash_read(ret, offset + 8, 1);
    /*if (!aflash_read(ret, offset, 9))
    {
        debug("Could not read DBFFS link header start at 0x%x.\n", address);
        free_dir_header(ret);
        return(NULL);
	}
	offset += 9;
	ret->name = db_malloc(ret->name_len + 1, "load_link_header ret->name"); 
	if (!aflash_read(ret->name, offset, ret->name_len))
    {
        debug("Could not read entry name at 0x%x.\n", offset);
        free_dir_header(ret);
        return(NULL);
	}
	offset += ret->name_len;
	ret->name[ret->name_len] = '\0';
	if (!aflash_read(&ret->entries, offset, sizeof(ret->entries)))
    {
        debug("Could not read number of directory entries at 0x%x.\n", offset);
        free_dir_header(ret);
        return(NULL);
	}*/
	return(ret);
}

/**
 * @brief Free memory used by a link header.
 * 
 * @param entry Pointer to the entry to free.
 */
static void ICACHE_FLASH_ATTR free_link_header(struct dbffs_link_hdr *entry)
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
 * @return A pointer to the link header.
 */
static struct dbffs_link_hdr ICACHE_FLASH_ATTR *load_link_header(unsigned int address)
{
	struct dbffs_link_hdr *ret;
	uint32_t offset = address;
	
	debug("Loading link header at 0x%x.\n", address);
	ret = db_malloc(sizeof(struct dbffs_link_hdr), "load_link_header ret");
	if (!ret)
	{
		error("Could not allocate memory for link header.\n");
		return(NULL);
	}
    if (!aflash_read(ret, offset, 9))
    {
        debug("Could not read DBFFS link header start at 0x%x.\n", address);
        free_link_header(ret);
        return(NULL);
	}
	offset += 9;
	ret->name = db_malloc(ret->name_len + 1, "load_link_header ret->name"); 
	if (!aflash_read(ret->name, offset, ret->name_len))
    {
        debug("Could not read entry name at 0x%x.\n", offset);
        free_link_header(ret);
        return(NULL);
	}
	offset += ret->name_len;
	ret->name[ret->name_len] = '\0';
	if (!aflash_read(&ret->target_len, offset, sizeof(ret->target_len)))
    {
        debug("Could not read target length at 0x%x.\n", offset);
        free_link_header(ret);
        return(NULL);
	}
	offset += sizeof(ret->target_len);
	ret->target = db_malloc(ret->target_len + 1, "load_link_header ret->target"); 
	if (!aflash_read(ret->target, offset, ret->target_len))
    {
        debug("Could not read target name at 0x%x.\n", offset);
        free_link_header(ret);
        return(NULL);
	}
	ret->target[ret->target_len] = '\0';
	return(ret);
}

/**
 * @brief Find a file header.
 */
struct dbffs_file_hdr ICACHE_FLASH_ATTR *dbffs_find_file_header(char *path)
{
	struct dbffs_generic_hdr *gen_hdr;
	struct dbffs_file_hdr *file_hdr;
	struct dbffs_link_hdr *link_hdr;
	struct dbffs_dir_hdr *dir_hdr;
	unsigned int hdr_off = 0;
	size_t link_path_len;
	char *link_path;
	char *path_off;
	
	debug("Finding file header for %s.\n", path);
	if (!path)
	{
		error("Path is NULL.\n");
		return(NULL);
	}
	//Root
	gen_hdr = load_generic_header(hdr_off);
	hdr_off = gen_hdr->next;
	//Skip initial slash
	path_off = path + 1;
	while (gen_hdr->next)
	{
		debug("Address 0x%x.\n", hdr_off);
		free_generic_header(gen_hdr);
		gen_hdr = load_generic_header(hdr_off);
		debug("Header address %p.\n", gen_hdr);
	
		if (gen_hdr)
		{
			debug(" Signature 0x%x.\n", gen_hdr->signature);
			debug(" Offset to next entry 0x%x.\n", gen_hdr->next);
			debug(" Name length %d.\n", gen_hdr->name_len);
			debug(" Name %s.\n", gen_hdr->name);
			//Check current name against current path entry.
			if (os_strncmp(gen_hdr->name, path_off, gen_hdr->name_len) == 0)
			{
				debug(" Entry name %s matches part of the path.\n",
				      gen_hdr->name);
				//Next part of the path.
				path_off += gen_hdr->name_len;
				switch (gen_hdr->signature)
				{
					case DBFFS_FILE_SIG:
						//Check if last match.
						if (*path_off == '\0')
						{
							debug("File found %s, full path %s.\n",
								  gen_hdr->name, path);
							free_generic_header(gen_hdr);
							file_hdr = load_file_header(hdr_off);
							return(file_hdr);
						}
						else
						{
							debug("Unexpected file found.\n");
						}
						hdr_off += gen_hdr->next;
						break;
					case DBFFS_DIR_SIG:
						//Skip slash.
						path_off++;
						//free_generic_header(gen_hdr);
						//dir_hdr = load_dir_header(hdr_off);
						//debug("Skipping %d directory entries.\n", dir_hdr->entries);
						hdr_off += gen_hdr->next;
						/*gen_hdr = load_generic_header(hdr_off);
						while ((gen_hdr->next) && (dir_hdr->entries > 0))
						{
							hdr_off += gen_hdr->next;
							free_generic_header(gen_hdr);
							dir_hdr->entries--;
							gen_hdr = load_generic_header(hdr_off);
							debug(".");
						}*/
						debug("\n Rest of search path %s.\n", path_off);
						break;
					case DBFFS_LINK_SIG:
						free_generic_header(gen_hdr);
						link_hdr = load_link_header(hdr_off);
						debug("Link target length %d.\n", link_hdr->target_len);
						debug("Link, target %s.\n", link_hdr->target);
						if (link_hdr->target[0] != '/')
						{
							error("Can only handle absoulte links.\n");
							free_link_header(link_hdr);
							return(NULL);
						}
						debug(" Remaining path %s.\n", path_off);
						link_path_len = link_hdr->target_len + strlen(path_off);
						debug(" Expanded link length %d.\n", link_path_len);
						link_path = db_malloc(link_path_len + 1, "link_path dbffs_find_file_header");
						memcpy(link_path, link_hdr->target, link_hdr->target_len);
						memcpy(link_path + link_hdr->target_len, path_off, link_path_len - link_hdr->target_len);
						link_path[link_path_len] = '\0';
						//free_link_header(link_hdr);
						debug( "Expanded link path %s.\n", link_path);				
						return(dbffs_find_file_header(link_path));
						break;
					default:
						warn("Unknown file entry type 0x%x.\n",
							 gen_hdr->signature);
						hdr_off += gen_hdr->next;
				}
			}
		}
		else
		{
			error("Could not read file system entry.\n");
		}
	}
	free_generic_header(gen_hdr);
	debug("File not found.\n");
	return(NULL);
}

/**
 * @brief Initialise the zip routines.
 */
void ICACHE_FLASH_ATTR init_dbffs(void)
{
    uint32_t signature;

    debug("Initialising DBBFS support.\n");
	debug(" Searching for file system.\n");
	//Find the root directory.
	while (fs_addr < MAX_FS_ADDR)
	{
		debug(" Trying 0x%x.\n", fs_addr);
		signature = load_signature(0);
		if (signature == DBFFS_DIR_SIG)
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
