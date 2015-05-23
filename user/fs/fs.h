/** 
 * @file fs.h
 *
 * @brief Routines for file access.
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
 
#ifndef FS_H
#define FS_H

#ifndef FS_MAX_OPEN_FILES
/**
 * @brief Maximum number of open files.
 */
#define FS_MAX_OPEN_FILES 8
#endif

/**
 * @brief End of file indicator.
 */
#define FS_EOF -1

/**
 * @brief Values for position origin.
 */
enum fs_seek_pos_e
{
    FS_SEEK_SET = 0,
    FS_SEEK_CUR,
    FS_SEEK_END
};

/**
 * @brief Type to hold origin position.
 */
typedef enum fs_seek_pos_e fs_seek_pos_t;

/**
 * @brief The type of a file handle.
 */
typedef int FS_FILE_H;

extern void fs_init(void);
extern FS_FILE_H fs_open(char *filename);
extern void fs_close(FS_FILE_H handle);
extern size_t fs_read(void *buffer, size_t size, size_t count, FS_FILE_H handle);
extern int fs_getc(FS_FILE_H handle);
extern char *fs_gets(char *str, size_t count, FS_FILE_H handle);
extern long fs_tell(FS_FILE_H handle);
extern long fs_size(FS_FILE_H handle);
extern int fs_seek(FS_FILE_H handle, long offset, fs_seek_pos_t origin);
extern int fs_eof(FS_FILE_H handle);

//~ int      ferror(FILE *);
//~ int      fgetpos(FILE *restrict, fpos_t *restrict);
//~ int      fileno(FILE *);
//~ void     flockfile(FILE *);
//~ FILE    *freopen(const char *restrict, const char *restrict, ILE *restrict);
//~ int      fscanf(FILE *restrict, const char *restrict, ...);
//~ int      fseeko(FILE *, off_t, int);
//~ int      fsetpos(FILE *, const fpos_t *);
//~ off_t    ftello(FILE *);
//~ int      ftrylockfile(FILE *);
//~ void     funlockfile(FILE *);
 
#endif //FS_H
