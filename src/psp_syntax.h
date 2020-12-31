/*
 *  Copyright (C) 2008 Ludovic Jacomme (ludovic.jacomme@gmail.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# ifndef _PSP_SYNTAX_H_
# define _PSP_SYNTAX_H_

#ifdef __cplusplus
extern "C" {
#endif

# define SYNTAX_NOCASE_SENS  0x00
# define SYNTAX_CASE_SENS    0x01

# define SYNTAX_HASH_SIZE   101

  typedef struct syntax_elem_t {
    struct syntax_elem_t *next;
    char                 *word;
    int                   length;
    u8                    color;
    u8                    flags;
  } syntax_elem_t;

  typedef struct syntax_list_t {
    struct syntax_list_t *next;
    char                 *title;
    char                 *delim;
    char                 *extention;
    syntax_elem_t        *hash_elem[SYNTAX_HASH_SIZE];
  } syntax_list_t;


#ifdef __cplusplus
}
#endif

  extern void psp_load_syntax();
  extern int psp_syntax_is_delim(char c);
  extern syntax_elem_t* psp_syntax_search_elem( char* a_line );
  extern char * psp_syntax_get_current();
  extern void psp_syntax_from_filename(char *filename);

# endif
