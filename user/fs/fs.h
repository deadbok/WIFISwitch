/** 
 * @file fs.h
 *
 * @brief Routines for file access.
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
 
#ifndef FS_H
#define FS_H

#ifndef FS_MAX_OPEN_FILES
#define FS_MAX_OPEN_FILES 8
#endif

typedef int FS_FILE_H;

extern void fs_init(void);
extern FS_FILE_H fs_open(const char *filename);
extern void fs_close(FS_FILE_H handle);
extern size_t fs_read(void *buffer, size_t size, size_t count, FS_FILE_H handle);


//~ int      fclose(FILE *);
//~ int      feof(FILE *);
//~ int      ferror(FILE *);
//~ int      fgetc(FILE *);
//~ int      fgetpos(FILE *restrict, fpos_t *restrict);
//~ char    *fgets(char *restrict, int, FILE *restrict);
//~ int      fileno(FILE *);
//~ void     flockfile(FILE *);
//~ FILE    *fopen(const char *restrict, const char *restrict);
//~ size_t   fread(void *restrict, size_t, size_t, FILE *restrict);
//~ FILE    *freopen(const char *restrict, const char *restrict, ILE *restrict);
//~ int      fscanf(FILE *restrict, const char *restrict, ...);
//~ int      fseek(FILE *, long, int);
//~ int      fseeko(FILE *, off_t, int);
//~ int      fsetpos(FILE *, const fpos_t *);
//~ long     ftell(FILE *);
//~ off_t    ftello(FILE *);
//~ int      ftrylockfile(FILE *);
//~ void     funlockfile(FILE *);
 
#endif //FS_H
