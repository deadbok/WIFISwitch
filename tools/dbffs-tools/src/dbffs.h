/** 
 * @file dbffs.h
 *
 * @brief DBFFS data structures and definitions
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
#ifndef DBFFS_H
#define DBFFS_H

/**
 * @brief DBFFS version.
 */
#define DBFFS_VERSION "0.0.2"
/**
 * @brief File header signature.
 */
#define DBFFS_FILE_SIG 0xDBFF500F
/**
 * @brief Directory header signature.
 */
#define DBFFS_DIR_SIG 0xDBFF500D
/**
 * @brief Link header signature.
 */
#define DBFFS_LINK_SIG 0xDBFF5001

/**
 * @brief Maximum file name length.
 */
#define DBFFS_MAX_FILENAME_LENGTH 256
/**
 * @brief Maximum path length.
 */
#define DBFFS_MAX_PATH_LENGTH 256
/**
 * @brief Maximum entries in file system. 
 */
#define DBFFS_MAX_ENTRIES 65536

/**
 * @brief File header.
 */
struct dbffs_file_hdr
{
	/**
	 * @brief Header signature.
	 */
	uint32_t signature;
	/**
	 * @brief Pointer to next entry.
	 */
	void *next;
	/**
	 * @brief File name length.
	 */
	uint8_t name_len;
	/**
	 * @brief File name.
	 */
	char *name;
	/**
	 * @brief Size of file data.
	 */
	uint32_t size;
	/**
	 * @brief The file data.
	 */
	uint8_t *data;
}  __attribute__ ((__packed__));

/**
 * @brief File header.
 */
struct dbffs_dir_hdr
{
	/**
	 * @brief Directory signature.
	 */
	uint32_t signature;
	/**
	 * @brief Pointer to next entry.
	 */
	void *next;
	/**
	 * @brief Number of file entries.
	 */
	uint8_t name_len;
	/**
	 * @brief File name.
	 */
	char *name;
	/**
	 * @brief Entries in the directory.
	 */
	uint16_t entries;
}  __attribute__ ((__packed__));

/**
 * @brief Link header.
 */
struct dbffs_link_hdr
{
	/**
	 * @brief Header signature.
	 */
	uint32_t signature;
	/**
	 * @brief Pointer to next entry.
	 */
	void *next;
	/**
	 * @brief Number of file entries.
	 */
	uint8_t name_len;
	/**
	 * @brief File name.
	 */
	char *name;
	/**
	 * @brief Length of the target path.
	 */
	uint8_t target_len;
	/**
	 * @brief Target path.
	 */
	char *target;
}  __attribute__ ((__packed__));

#endif //DBFFS
