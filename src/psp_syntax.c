/*
 *  Copyright (C) 2008 Ludovic Jacomme (ludovic.jacomme@gmail.com)
 *
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Based on the source code of PelDet written by Danzel A.
*/

#include <pspkernel.h>
#include <psppower.h>
#include <pspctrl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL.h>

#include "psp_global.h"
#include "psp_sdl.h"
#include "psp_danzeff.h"
#include "psp_irkeyb.h"
#include "psp_menu.h"
#include "psp_fmgr.h"
#include "psp_editor.h"
#include "psp_syntax.h"

  static syntax_list_t*  loc_head_syntax = 0;
  static syntax_list_t*  loc_curr_syntax = 0;

# define SYNTAX_HASH_MULT   314159
# define SYNTAX_HASH_PRIME  516595003

static int
psp_syntax_hash_key( char* a_word, int len )
{
  int hash_key = 0;
  while ( len-- > 0 ) {
    unsigned char c = *a_word++;
    c = tolower(c);
    hash_key += ( hash_key ^ ( hash_key >> 1 ) );
    hash_key += SYNTAX_HASH_MULT * c;
    while ( hash_key >= SYNTAX_HASH_PRIME ) {
      hash_key -= SYNTAX_HASH_PRIME;
    }
  }
  hash_key %= SYNTAX_HASH_SIZE;
  return hash_key;
}

static syntax_list_t* 
psp_syntax_create( char* title )
{
  syntax_list_t* new_syntax = (syntax_list_t*)malloc( sizeof(syntax_list_t) );
  memset(new_syntax, 0, sizeof(syntax_list_t));
  new_syntax->title = strdup( title );
  new_syntax->delim = strdup(" \t\n\r");
  new_syntax->next  = loc_head_syntax;
  loc_head_syntax = new_syntax;
  return new_syntax;
}

static syntax_elem_t*
psp_syntax_add_word( syntax_list_t* a_syntax, char* a_word, u8 color, u8 flags)
{
  syntax_elem_t* new_elem = (syntax_elem_t *)malloc( sizeof(syntax_elem_t) );
  int length  = strlen( a_word );
  int hash_id = psp_syntax_hash_key( a_word, length );

  new_elem->next   = a_syntax->hash_elem[hash_id];
  new_elem->word   = strdup( a_word );
  new_elem->color  = color;
  new_elem->length = length;
  new_elem->flags  = flags;

  a_syntax->hash_elem[hash_id] = new_elem;
  return new_elem;
}

char *
psp_syntax_get_current()
{
  return loc_curr_syntax->title;
}

syntax_list_t*
psp_syntax_find_current(char *title)
{
  syntax_list_t* scan_list;
  for (scan_list = loc_head_syntax; scan_list != 0; scan_list = scan_list->next) {
    if (!strcmp(scan_list->title, title)) {
      loc_curr_syntax = scan_list;
      return loc_curr_syntax;
    }
  }
  loc_curr_syntax = loc_head_syntax;
  return 0;
}

char*
psp_syntax_go_next()
{
  if (loc_curr_syntax->next) {
    loc_curr_syntax = loc_curr_syntax->next;
  }
  return loc_curr_syntax->title;
}

char*
psp_syntax_go_previous()
{
  syntax_list_t* scan_list;
  for (scan_list = loc_head_syntax; scan_list != 0; scan_list = scan_list->next) {
    if (scan_list->next == loc_curr_syntax) {
      loc_curr_syntax = scan_list;
      return loc_curr_syntax->title;
    }
  }
  return loc_curr_syntax->title;
}

int
psp_syntax_is_delim(char c)
{
  int   index;
  char *delim = loc_curr_syntax->delim;
  for (index = 0; delim[index]; index++) {
    if (c == delim[index]) return 1;
  }
  return 0;
}

syntax_elem_t*
psp_syntax_search_elem( char* a_line )
{
  syntax_elem_t *scan_elem;
  int            length;
  int            hash_id;

  if (! a_line) return 0;

  for (length = 0; a_line[length]; length++) {
    if (psp_syntax_is_delim(a_line[length])) break;
  }
  if (length == 0) return 0;

  hash_id = psp_syntax_hash_key( a_line, length );
  scan_elem = loc_curr_syntax->hash_elem[ hash_id ];
  while (scan_elem != 0) {
    if (scan_elem->length == length) {
      if (scan_elem->flags == SYNTAX_NOCASE_SENS) {
        if (!strncasecmp(scan_elem->word, a_line, length)) {
          return scan_elem;
        }
      } else {
        if (!strncmp(scan_elem->word, a_line, length)) {
          return scan_elem;
        }
      }
    }
    scan_elem = scan_elem->next;
  }
  return 0;
}

static syntax_elem_t*
psp_syntax_reverse_elem( syntax_elem_t* head_elem )
{
  syntax_elem_t* p;
  syntax_elem_t* q = 0;

  if (! head_elem) return head_elem;

  while ((p = head_elem->next)) {
    head_elem->next = q;
    q = head_elem;
    head_elem = p;
  }
  head_elem->next = q;
  return head_elem;
}

void
psp_syntax_from_filename(char *filename)
{
  syntax_list_t* scan_list;
  int   length;
  char *ext_pos = strrchr(filename, '.');
  if (! ext_pos) return;

  length = strlen(filename);
  ext_pos++;

  if ((ext_pos - filename) < (length - 3)) return;

  for (scan_list = loc_head_syntax; scan_list != 0; scan_list = scan_list->next) {
    if (!scan_list->extention || !scan_list->extention[0]) continue;

    char* scan_ext = strtok(scan_list->extention, ".");
    do {
      if (scan_ext && scan_ext[0]) {
        if (!strcmp(ext_pos, scan_ext)) {
          loc_curr_syntax = scan_list;
          return;
        }
      }
    } while (scan_ext = strtok(0, "."));
  }
}

void
psp_load_syntax()
{
  char  FileName[MAX_PATH+1];
  char  Buffer[512];
  char  Delim[128];
  FILE* FileDesc;
  char* Scan;
  int   error;

  u8 color = COLOR_IMAGE;
  syntax_list_t *curr_syntax;

  loc_curr_syntax = psp_syntax_create("none");
  curr_syntax = loc_curr_syntax;

  snprintf(FileName, MAX_PATH, "%s/syntax.cfg", PSPWRITE.psp_homedir);
  error = 0;
  FileDesc = fopen(FileName, "r");
  if (FileDesc == (FILE *)0 ) return;

  while (fgets(Buffer,512, FileDesc) != (char *)0) {
    Scan = strchr(Buffer,'\n');
    if (Scan) *Scan = '\0';
    /* For this #@$% of windows ! */
    Scan = strchr(Buffer,'\r');
    if (Scan) *Scan = '\0';
    if (Buffer[0] == '#') continue;

    Scan = strchr(Buffer,'=');
    if (! Scan) continue;
    *Scan = '\0';
    Scan++;

    if (!strcasecmp(Buffer, "title")) {
      curr_syntax = psp_syntax_create( Scan );
      color = COLOR_IMAGE;
    } else 
    if (!strcasecmp(Buffer, "extention")) {
      if (curr_syntax) {
        if (curr_syntax->extention) {
          free(curr_syntax->extention);
        }
        curr_syntax->extention = strdup( Scan );
      }
    } else 
    if (!strcasecmp(Buffer, "delim")) {
      if (curr_syntax) {
        strcpy(Delim, curr_syntax->delim);
        strcat(Delim, Scan);
        free(curr_syntax->delim);
        curr_syntax->delim = strdup( Delim );
      }
    } else
    if (!strcasecmp(Buffer, "word")) {
      if (curr_syntax) {
        psp_syntax_add_word( curr_syntax, Scan, color, SYNTAX_CASE_SENS );
      }
    } else
    if (!strcasecmp(Buffer, "iword")) {
      if (curr_syntax) {
        psp_syntax_add_word( curr_syntax, Scan, color, SYNTAX_NOCASE_SENS );
      }
    } else
    if (!strcasecmp(Buffer, "color")) {
      color = atoi(Scan) % EDITOR_MAX_COLOR;
    }
  }

  /* reverse all list */
  syntax_list_t *scan_syn;
  int hash_id;
  for (scan_syn = loc_head_syntax; scan_syn != 0; scan_syn= scan_syn->next) {
    for (hash_id = 0; hash_id < SYNTAX_HASH_SIZE; hash_id++) {
      scan_syn->hash_elem[ hash_id ] = psp_syntax_reverse_elem( scan_syn->hash_elem[ hash_id ] );
    }
  }

  fclose(FileDesc);
}
