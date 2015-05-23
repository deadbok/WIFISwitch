#ifndef ZIP_H
#define ZIP_H

#include "c_types.h"

/**
 * @brief Signature of the file header.
 */
#define ZIP_FILE_HEADER_SIGNATURE 0x04034b50
/**
 * @brief The size of the local file header in the ZIP data.
 */
#define ZIP_REAL_FILE_HEADER_SIZE 30

/**
 * @brief Internal file header structure.
 * 
 * Describes a file inside the ZIP package including pointers for the name, and
 * extra data.
 */
struct __attribute__ ((__packed__)) zip_file_hdr 
{
    /**
     * @brief General purpose bit flag.
     */
    uint16_t flags;
    /**
     * @brief Compression method.
     */
    uint16_t compression_method;
    /**
     * @breif Last modification time.
     */
    uint16_t last_mod_time;
    /**
     * @brief Last modification date.
     */
    uint16_t last_mod_date;
    /**
     * @brief Size of uncompressed data.
     */
    uint32_t uncompressed_size;
    /**
     * @brief Length of filename.
     */ 
    uint16_t filename_len;
    /**
     * @brief Length of extra data.
     */
    uint16_t extra_len;
    /**
     * @brief The name of the file.
     */
    char *filename;
        /**
     * @brief Offset of the data, .
     */
    uint32_t data_pos;
};

extern struct zip_file_hdr *zip_find_file_header(char *path);
extern bool zip_is_dir(char *path);
extern void zip_free_header(struct zip_file_hdr *file_hdr);

#endif //ZIP_H
