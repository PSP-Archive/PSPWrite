/*
 *  Copyright (C) 2007 Ludovic Jacomme (ludovic.jacomme@gmail.com)
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspiofilemgr.h>


#include "psp_global.h"
#include "psp_sdl.h"
#include "psp_danzeff.h"
#include "psp_irkeyb.h"
#include "psp_menu.h"
#include "psp_fmgr.h"
#include "psp_editor.h"
#include "psp_syntax.h"

# define EDITOR_MAX_LINES    100000
# define EDITOR_MAX_WIDTH     65534

/* NEOL : No end of line */
# define EDITOR_NEOL_MASK        0x01

# define psp_editor_is_neol(L)  ((L)->flags & EDITOR_NEOL_MASK)
# define psp_editor_set_neol(L) ((L)->flags |= EDITOR_NEOL_MASK)
# define psp_editor_clear_neol(L) ((L)->flags &= ~EDITOR_NEOL_MASK)

  typedef struct line_t {
    char* text;
    u16   width; 
    u16   max_width;
    u8    flags;
  } line_t;

  typedef struct text_t {
    line_t*   lines[EDITOR_MAX_LINES];

    char*     wrap_buffer;
    int       wrap_buffer_size;

    int       max_width;
    int       max_lines;

    int       num_lines;
    int       top_line;
    int       left_col;
    int       curr_line;
    int       curr_col;

    u32       last_blink;
    char      cursor_blink;
    char      cursor_moving;
    char      cursor_end;
    int       last_col;

    int       sel_from_col;
    int       sel_from_line;
    int       sel_to_col;
    int       sel_to_line;
    int       sel_mode;
    
  } text_t;

  typedef struct line_list_t {

    struct line_list_t* next;
    line_t*             line;

  } line_list_t;

  typedef struct clip_t {
    line_list_t*  head_list;
    line_list_t*  last_list;
    int           number_lines;
  } clip_t;

  static text_t* g_text;
  static clip_t* g_clip;

  int editor_colors[EDITOR_MAX_COLOR];

  char* editor_colors_name[EDITOR_MAX_COLOR] = {
     "white" ,
     "black" ,
     "dark blue" ,
     "green" ,
     "red" ,
     "brown" ,
     "magenta" ,
     "orange" ,
     "yellow" ,
     "bright green" ,
     "cyan" ,
     "bright blue" ,
     "blue" ,
     "pink" ,
     "gray" ,
     "bright gray",
     "image"
  };

# define EDITOR_COMMAND_WIDTH    60
# define EDITOR_COMMAND_HISTORY  25

  static int  command_mode = 0;
  static char command_line[EDITOR_COMMAND_WIDTH + 1];
  static int  command_pos = 0;
  static char command_result[EDITOR_COMMAND_WIDTH + 1];

  static char command_history[EDITOR_COMMAND_HISTORY+1][EDITOR_COMMAND_WIDTH+1];
  static int  command_history_size = 0;
  static int  command_history_pos  = 0;

# define EDITOR_MAX_COMMAND 2

  static char *command_list[EDITOR_MAX_COMMAND+1] = {
    "", "/", "?"
  };
  static int command_curr_list = 0;

 extern void psp_editor_cut();
 extern int  psp_editor_rewrap_buffer_line(int from_line_id, int abs_curr_pos);
 extern void psp_editor_del_line_from(int line_id, int num_lines);

static inline int 
my_isspace(char c)
{
  return ((c == ' ') || (c == '\t') || (c == '\f') || (c == '\v'));
}

#if 1
static void
psp_editor_dump_all_lines()
{
  int line_id;
  fprintf(stdout, "> psp_editor_dump_all_lines\n");
  for (line_id = 0; line_id < g_text->num_lines; line_id++) {
    line_t* a_line = g_text->lines[line_id];
    fprintf(stdout, "[%04d] ", line_id);
    if (a_line && a_line->width) {
      if (a_line->width != strlen(a_line->text)) {
        fprintf(stdout, "ERROR w=%d '%s'\n", a_line->width, a_line->text);
      } else {
        fprintf(stdout, "w=%d '%s'\n", a_line->width, a_line->text);
      }

    } else {
      fprintf(stdout, "\n");
    }
  }
  fprintf(stdout, "< psp_editor_dump_all_lines\n");
}
#endif

line_t*
psp_editor_alloc_line()
{
  line_t* a_line = (line_t*)malloc( sizeof(line_t) );
  memset(a_line, 0, sizeof( line_t ));
  return a_line;
}

line_t*
psp_editor_add_line(int line_id, char *buffer)
{
  if (line_id >= g_text->max_lines) return 0;

  line_t* a_line = g_text->lines[line_id];
  int length = strlen( buffer );

  if (! a_line) {
    a_line = psp_editor_alloc_line();
    g_text->lines[line_id] = a_line;
  }

  if (length < a_line->max_width) {
    strcpy(a_line->text, buffer);
    a_line->width = length;
    a_line->text[a_line->width] = 0;
  } else {
    if (a_line->text) free(a_line->text);
    a_line->width     = length;
    a_line->max_width = length;
    a_line->text = strdup( buffer );
    a_line->text[a_line->width] = 0;
  }

  if (line_id >= g_text->num_lines) {
    g_text->num_lines = line_id + 1;
  }
  PSPWRITE.is_modified = 1;
  return a_line;
}

line_t* 
psp_editor_insert_line(int line_id, char* text)
{
  if (g_text->num_lines >= g_text->max_lines) return 0;

  PSPWRITE.is_modified = 1;

  int delta = g_text->num_lines - line_id;
  if (delta > 0) {
    int index = 0;
    for (index = g_text->num_lines - 1; index >= line_id; index--) {
      g_text->lines[index + 1] = g_text->lines[index];
    }
    g_text->lines[line_id] = 0;
  }
  line_t* a_line = psp_editor_add_line(line_id, text);
  g_text->num_lines++;

  psp_editor_update_column(1);
  psp_editor_update_line();

  return a_line;
}


int
psp_editor_rewrap_buffer_line(int from_line_id, int abs_curr_pos)
{
# if 0
  fprintf(stdout, "\ncurr_pos: %d %d\n", g_text->curr_line, g_text->curr_col);
# endif
  char* begin_wrap  = g_text->wrap_buffer;
  int   length_left = strlen(begin_wrap);
  int   length_init = length_left;

# if 0
fprintf(stdout, "psp_editor_rewrap_buffer_line from=%d left=%d abs_curr=%d '%s'\n", 
        from_line_id, length_left, abs_curr_pos, begin_wrap);
# endif

  line_t* a_last_line = 0;
  int a_line_id = from_line_id;
  while (length_left > 0) {

    if (length_left > PSPWRITE.wrap_w) {
      /* find first space starting from wrap_w */
      int first_space = PSPWRITE.wrap_w;
      while (first_space > 0) {
        if (my_isspace(begin_wrap[first_space])) break;
        first_space--;
      }
      /* no space found */
      if (! first_space) {
        first_space = PSPWRITE.wrap_w;
      }

      char save_char = begin_wrap[first_space];
      begin_wrap[first_space] = 0;
      a_last_line = psp_editor_insert_line(a_line_id, begin_wrap);
      if (a_last_line) psp_editor_set_neol(a_last_line);
      begin_wrap[first_space] = save_char;

      if (abs_curr_pos >= 0) {
        /* if abs_curr_pos is between abs_begin and abs_end */
        int abs_begin = length_init - length_left;
        int abs_end   = abs_begin + first_space;
        if ((abs_curr_pos >= abs_begin) && (abs_curr_pos <= abs_end)) {
          /* found it ! */
          if (a_last_line) {
            g_text->curr_line = a_line_id;
            g_text->curr_col  = abs_curr_pos - abs_begin;
            if (g_text->curr_col > a_last_line->width) {
              g_text->curr_col = a_last_line->width;
            }
          } else {
            g_text->curr_line = a_line_id;
            g_text->curr_col  = 0;
          }
# if 0
  fprintf(stdout, "\nnew curr_pos: %d %d\n", g_text->curr_line, g_text->curr_col);
# endif
        }
      }

      length_left -= first_space;
      begin_wrap += first_space;

      while ((length_left > 0) && my_isspace(begin_wrap[0])) {
        begin_wrap++;
        length_left--;
      }

    } else {
      a_last_line = psp_editor_insert_line(a_line_id, begin_wrap);
      if (a_last_line) psp_editor_set_neol(a_last_line);

      if (abs_curr_pos >= 0) {
        /* if abs_curr_pos is between abs_begin and abs_end */
        int abs_begin = length_init - length_left;
        int abs_end   = length_init;
        if ((abs_curr_pos >= abs_begin) && (abs_curr_pos <= abs_end)) {
          /* found it ! */
          if (a_last_line) {
            g_text->curr_line = a_line_id;
            g_text->curr_col  = abs_curr_pos - abs_begin;
            if (g_text->curr_col > a_last_line->width) {
              g_text->curr_col = a_last_line->width;
            }
          } else {
            g_text->curr_line = a_line_id;
            g_text->curr_col  = 0;
          }
        }
      }
      length_left = 0;
    }
    a_line_id++;
  }
  if (a_last_line) psp_editor_clear_neol(a_last_line);

  return a_line_id;
}

void
psp_editor_rewrap_line(int rewrap_line_id)
{
  int from_line_id = rewrap_line_id;
  line_t* a_line_from = g_text->lines[from_line_id];
  if (! a_line_from) return;

  int a_line_id = rewrap_line_id;
  int to_line_id = rewrap_line_id;

  if (psp_editor_is_neol(a_line_from)) {

    /* scan for first neol line */
    while (a_line_from && psp_editor_is_neol(a_line_from)) {
      from_line_id = a_line_id;
      if (a_line_id > 0) a_line_id--;
      else break;
      a_line_from = g_text->lines[a_line_id];
    }

    /* scan for last neol line */
    a_line_id = to_line_id;
    line_t* a_line_to = g_text->lines[to_line_id];

    while (a_line_to && psp_editor_is_neol(a_line_to)) {
      to_line_id = a_line_id;
      if (a_line_id < g_text->num_lines) a_line_id++;
      else break;
      a_line_to = g_text->lines[a_line_id];
    }

    if (a_line_to && !psp_editor_is_neol(a_line_to)) {
      to_line_id = a_line_id;
    }

  } else {

    /* scan for first neol line */
    a_line_id = from_line_id;
    if (a_line_id > 0) a_line_id--;
    a_line_from = g_text->lines[a_line_id];

    while (a_line_from && psp_editor_is_neol(a_line_from)) {
      from_line_id = a_line_id;
      if (a_line_id > 0) a_line_id--;
      else break;
      a_line_from = g_text->lines[a_line_id];
    }
  }

# if 0
fprintf(stdout, "psp_editor_rewrap_line from=%d to=%d\n", from_line_id, to_line_id);
# endif

  int abs_curr_pos = -1;
  int length = 0;
  for (a_line_id = from_line_id; a_line_id <= to_line_id; a_line_id++) {
    line_t* a_line = g_text->lines[a_line_id];
    if (a_line_id == g_text->curr_line) {
      abs_curr_pos = length;
      if (a_line) {
        if (g_text->curr_col > a_line->width) abs_curr_pos += a_line->width;
        else                                  abs_curr_pos += g_text->curr_col;
      }
    }
    if (a_line) length += a_line->width + 1;
  }
  if (! length) return;

  if (g_text->wrap_buffer_size < length) {
    g_text->wrap_buffer = realloc(g_text->wrap_buffer, length + 1);
    g_text->wrap_buffer_size = length;
  }

  char* scan_buffer = g_text->wrap_buffer;
  scan_buffer[0] = 0;
  for (a_line_id = from_line_id; a_line_id <= to_line_id; a_line_id++) {
    line_t* a_line = g_text->lines[a_line_id];
    if (a_line && a_line->width) {
      strcpy(scan_buffer, a_line->text);
      scan_buffer += a_line->width;
      if (a_line_id < to_line_id) {
        strcpy(scan_buffer, " ");
        scan_buffer++;
      }
    }
  }
# if 0
fprintf(stdout, "AVANT wrap_buffer='%s'\n", g_text->wrap_buffer);
psp_editor_dump_all_lines();
# endif

  /* delete lines between line_from and line_to */
  int num_lines = 1 + to_line_id - from_line_id;
  psp_editor_del_line_from(from_line_id, num_lines);

  /* rewrap all deleted lines */
  psp_editor_rewrap_buffer_line(from_line_id, abs_curr_pos);

# if 0
fprintf(stdout, "APRES wrap_buffer='%s'\n", g_text->wrap_buffer);
psp_editor_dump_all_lines();
# endif
}

void
psp_editor_rewrap_curr_line()
{
  psp_editor_rewrap_line(g_text->curr_line);
  psp_editor_update_column(1);
  psp_editor_update_line();
  g_text->sel_mode = 0;
}

void
psp_editor_resize_line(line_t* a_line, int new_width)
{
  PSPWRITE.is_modified = 1;
  if (new_width > g_text->max_width) new_width = g_text->max_width;
  if (new_width > a_line->max_width) {
    a_line->text = realloc(a_line->text, new_width + 1);
    a_line->max_width = new_width;
  }
}

void
psp_editor_reset_text()
{
  int line_id;
  for (line_id = 0; line_id < g_text->num_lines; line_id++) {
    line_t* a_line = g_text->lines[line_id];
    if (a_line) {
      if (a_line->text) {
        free(a_line->text);
      }
      memset(a_line, 0, sizeof(line_t));
    }
  }
  g_text->num_lines = 0;
  g_text->top_line = 0;
  g_text->left_col = 0;
  g_text->curr_line = 0;
  g_text->curr_col = 0;
  g_text->cursor_moving = 0;
  g_text->cursor_blink = 0;
  g_text->cursor_end = 0;
  g_text->last_col = 0;

  g_text->sel_mode      = 0;
  g_text->sel_from_line = 0;
  g_text->sel_to_line   = 0;
  g_text->sel_from_col  = 0;
  g_text->sel_to_col    = 0;

  psp_editor_add_line(0, "");
}

void
psp_editor_update_column(int update_last)
{
  if (g_text->curr_col >= g_text->max_width) {
    g_text->curr_col = g_text->max_width - 1;
  }
  if ((g_text->curr_col - g_text->left_col) >= PSPWRITE.screen_w) {
    g_text->left_col = g_text->curr_col - PSPWRITE.screen_w + 1;
  }
  if (g_text->curr_col < 0) {
    g_text->curr_col = 0;
  }
  if (g_text->curr_col < g_text->left_col) {
    g_text->left_col = g_text->curr_col - PSPWRITE.screen_w / 3;
    if (g_text->left_col < 0) g_text->left_col = 0;
  }

  line_t* a_line = g_text->lines[g_text->curr_line];
  if (a_line && a_line->width) {
    if (a_line->width != g_text->curr_col) g_text->cursor_end = 0;
    if (! g_text->cursor_end) {
      if (update_last) {
        g_text->last_col = g_text->curr_col;
      }
    }
  }
}

void
psp_editor_sel_mode()
{
  g_text->sel_mode = ! g_text->sel_mode;
  if (g_text->sel_mode) {
    g_text->sel_from_line = g_text->curr_line;
    g_text->sel_to_line   = g_text->curr_line;
    g_text->sel_from_col  = g_text->curr_col;
    g_text->sel_to_col    = g_text->curr_col;
  }
  psp_kbd_wait_no_button();
}

void
psp_editor_update_sel()
{
  if (g_text->sel_mode) {
    if (g_text->curr_line < g_text->sel_from_line) {
      g_text->sel_from_line = g_text->curr_line;
      g_text->sel_from_col  = g_text->curr_col;
    } else 
    if (g_text->curr_line > g_text->sel_to_line) {
      g_text->sel_to_line = g_text->curr_line;
      g_text->sel_to_col  = g_text->curr_col;
    } else {
    
      if (g_text->curr_line == g_text->sel_from_line) {
        if (g_text->curr_col < g_text->sel_from_col) g_text->sel_from_col = g_text->curr_col;
      }
      if (g_text->curr_line == g_text->sel_to_line) {
        if (g_text->curr_col > g_text->sel_to_col) g_text->sel_to_col = g_text->curr_col;
      }
      if ((g_text->curr_line == (g_text->sel_to_line-1)) &&
          (g_text->curr_line >  (g_text->sel_from_line))) {
        g_text->sel_to_line = g_text->curr_line;
        g_text->sel_to_col  = g_text->curr_col;
      } else
      if ((g_text->curr_line == (g_text->sel_from_line+1)) &&
          (g_text->curr_line <  (g_text->sel_to_line))) {
        g_text->sel_from_line = g_text->curr_line;
        g_text->sel_from_col  = g_text->curr_col;
      }
    }
  }
}

void
psp_editor_update_line()
{
  if (g_text->curr_line >= g_text->num_lines) {
    g_text->curr_line = g_text->num_lines - 1;
  }
  if ((g_text->curr_line - g_text->top_line) >= PSPWRITE.screen_h) {
    g_text->top_line = g_text->curr_line - PSPWRITE.screen_h + 1;
  }
  if (g_text->curr_line < 0) {
    g_text->curr_line = 0;
  }
  if (g_text->curr_line < g_text->top_line) {
    g_text->top_line = g_text->curr_line;
  }
}


void
psp_editor_goto_line_down()
{
  g_text->cursor_moving = 1;

  if (PSPWRITE.move_on_text) {

    if ((g_text->curr_line + 1) < g_text->num_lines) {
      g_text->curr_line++;
      line_t* a_line_next = g_text->lines[g_text->curr_line];
      if (! a_line_next) {
        g_text->curr_col = 0;
      } else
      if (g_text->cursor_end || (g_text->curr_col > a_line_next->width)) {
        g_text->curr_col = a_line_next->width;
        if ((g_text->last_col) && (g_text->curr_col > g_text->last_col)) {
          g_text->curr_col = g_text->last_col;
        }
      } else {
        if (g_text->last_col <= a_line_next->width) {
          g_text->curr_col = g_text->last_col;
        } else {
          g_text->curr_col = a_line_next->width;
        }
      }
    }

  } else {
    g_text->curr_line++;
  }
  psp_editor_update_column(0);
  psp_editor_update_line();
  psp_editor_update_sel();
}

void
psp_editor_goto_line_up()
{
  g_text->cursor_moving = 1;

  if (PSPWRITE.move_on_text) {

    if (g_text->curr_line > 0) {
      g_text->curr_line--;
      line_t* a_line_prev = g_text->lines[g_text->curr_line];
      if (! a_line_prev) {
        g_text->curr_col = 0;
      } else
      if (g_text->cursor_end || (g_text->curr_col > a_line_prev->width)) {
        g_text->curr_col = a_line_prev->width;
        if ((g_text->last_col) && (g_text->curr_col > g_text->last_col)) {
          g_text->curr_col = g_text->last_col;
        }
      } else {
        if (g_text->last_col <= a_line_prev->width) {
          g_text->curr_col = g_text->last_col;
        }
      }
    }
  } else {
    g_text->curr_line--;
  }
  psp_editor_update_column(0);
  psp_editor_update_line();
  psp_editor_update_sel();
}

void
psp_editor_goto_col_left()
{
  g_text->cursor_moving = 1;

  if (PSPWRITE.move_on_text) {

    line_t* a_line = g_text->lines[g_text->curr_line];
    if (! a_line) {
      g_text->curr_col = 0;
    } else {
      if ((g_text->curr_col == 0) && (g_text->curr_line > 0)) {
        g_text->curr_line--;
        line_t* a_line_prev = g_text->lines[g_text->curr_line];
        if (a_line_prev) {
          g_text->curr_col = a_line_prev->width;
          g_text->last_col = g_text->max_width - 1;
        } else {
          g_text->curr_col = 0;
        }
      } else {
        g_text->curr_col--;
      }
    }

  } else {
    g_text->curr_col--;
  }
  psp_editor_update_line();
  psp_editor_update_sel();
  psp_editor_update_column(1);
}

void
psp_editor_goto_col_right()
{
  g_text->cursor_moving = 1;

  if (PSPWRITE.move_on_text) {

    line_t* a_line = g_text->lines[g_text->curr_line];
    if (! a_line) {
      if ((g_text->curr_line + 1) < g_text->num_lines) {
        g_text->curr_line++;
        g_text->curr_col = 0;
      }
    } else {
      if ( (g_text->curr_col == a_line->width          ) && 
           ((g_text->curr_line + 1) < g_text->num_lines) ) {
        g_text->curr_line++;
        g_text->curr_col = 0;
      } else {
        g_text->curr_col++;
      }
    }

  } else {
    g_text->curr_col++;
  }
  psp_editor_update_line();
  psp_editor_update_sel();
  psp_editor_update_column(1);
}

void
psp_editor_goto_col_begin()
{
  g_text->cursor_moving = 1;
  g_text->curr_col = 0;
  psp_editor_update_column(1);
}

void
psp_editor_goto_col_end()
{
  g_text->cursor_moving = 1;
  line_t* a_line = g_text->lines[g_text->curr_line];
  if (a_line) {
    g_text->curr_col = a_line->width;
    g_text->last_col = g_text->max_width - 1;
    g_text->cursor_end = 1;
  } else {
    g_text->curr_col = 0;
  }
  psp_editor_update_column(1);
  psp_editor_update_sel();
}

void
psp_editor_goto_first_line()
{
  g_text->cursor_moving = 1;
  g_text->curr_col = 0;
  g_text->curr_line = 0;
  psp_editor_update_column(0);
  psp_editor_update_line();
  psp_editor_update_sel();
}

void
psp_editor_goto_last_line()
{
  g_text->cursor_moving = 1;
  g_text->curr_col = 0;
  g_text->curr_line = g_text->num_lines;
  psp_editor_update_column(0);
  psp_editor_update_line();
  psp_editor_update_sel();
}

void
psp_editor_goto_page_down()
{
  g_text->cursor_moving = 1;
  g_text->curr_line += PSPWRITE.screen_h;
  psp_editor_update_line();
  psp_editor_update_sel();
}

void
psp_editor_goto_page_up()
{
  g_text->cursor_moving = 1;
  g_text->curr_line -= PSPWRITE.screen_h;
  psp_editor_update_line();
  psp_editor_update_sel();
}

void
psp_editor_goto_word_left()
{
  g_text->cursor_moving = 1;
  line_t* a_line = g_text->lines[g_text->curr_line];
  if (a_line && a_line->text) {
    int index = g_text->curr_col;
    if (index > 0) index--;
    while ((index > 0) && (a_line->text[index] == ' ')) index--;
    while ((index > 0) && (a_line->text[index] != ' ')) index--;
    if (a_line->text[index] == ' ') index++;
    g_text->curr_col = index;
    
  } else g_text->curr_col = 0;
  psp_editor_update_column(1);
  psp_editor_update_sel();
}

void
psp_editor_goto_word_right()
{
  g_text->cursor_moving = 1;
  line_t* a_line = g_text->lines[g_text->curr_line];
  if (a_line && a_line->text) {
    int index = g_text->curr_col;
    while ((index < a_line->width) && (a_line->text[index] != ' ')) index++;
    while ((index < a_line->width) && (a_line->text[index] == ' ')) index++;
    g_text->curr_col = index;
    
  } else g_text->curr_col = 0;
  psp_editor_update_column(1);
  psp_editor_update_sel();
}

void
psp_editor_insert_char(int line_id, int col_id, int c)
{
  PSPWRITE.is_modified = 1;

  uchar c1 = (c >> 8) & 0xff;
  uchar c2 = c & 0xff;
  if ((c1 == 0xc2) || (c1 == 0xc3)) {
    uchar new_c = psp_convert_utf8_to_iso_8859_1(c1, c2);
    if (new_c) { c = new_c; }
  }

  line_t* a_line = g_text->lines[line_id];
  if (! a_line) {
# if 0
  fprintf(stdout, "$$$ insert_char line is null\n");
# endif
    a_line = psp_editor_add_line(line_id, "");
  }
# if 0
  fprintf(stdout, "### insert_char '%s' %d\n", a_line->text, a_line->width);
# endif

  if (col_id >= a_line->width) {
    psp_editor_resize_line(a_line, 1 + ((col_id * 3) / 2) );
    while (a_line->width < col_id) {
      a_line->text[a_line->width++] = ' ';
    }
    a_line->text[col_id] = c;
    a_line->width++;
    a_line->text[a_line->width] = 0;
  } else {
    if ((a_line->width + 1) >= a_line->max_width) {
      psp_editor_resize_line(a_line, 1 + ((a_line->width * 3) / 2) );
    }
    a_line->width++;
    int index = a_line->width;
    while (index > col_id) {
      a_line->text[index] = a_line->text[index - 1];
      index--;
    }
    a_line->text[col_id] = c;
    a_line->text[a_line->width] = 0;
  }

# if 0
  fprintf(stdout, "!!! insert_char '%s' %d\n", a_line->text, a_line->width);
# endif
  g_text->sel_mode = 0;
}

void
psp_editor_rewrap_if_needed()
{
  if (PSPWRITE.wrap_mode) {
    psp_editor_rewrap_line(g_text->curr_line);
    psp_editor_update_column(1);
    psp_editor_update_line();
    g_text->sel_mode = 0;
  }
}

void
psp_editor_insert_curr_char(int c)
{
  if ((c == '\t') && PSPWRITE.expand_tab) {
    int tab;
    for (tab = 0; tab < PSPWRITE.tab_stop; tab++) {
      psp_editor_insert_curr_char(' ');
    }
  } else {
    psp_editor_insert_char(g_text->curr_line, g_text->curr_col, c);
    g_text->curr_col++;
  }
  psp_editor_rewrap_if_needed();
  psp_editor_update_column(1);
  g_text->sel_mode = 0;
}

void
psp_editor_split_line(int line_id, int col_id)
{
  if (g_text->num_lines >= g_text->max_lines) return;

  PSPWRITE.is_modified = 1;

  line_t* a_line = g_text->lines[line_id];
  if (! a_line) {
    a_line = psp_editor_add_line(line_id, "");
  }
  int n_line_id = line_id + 1;
  int delta = g_text->num_lines - line_id - 1;
  if (delta > 0) {
    int index = 0;
    for (index = g_text->num_lines - 1; index > line_id; index--) {
      g_text->lines[index + 1] = g_text->lines[index];
    }
    g_text->lines[n_line_id] = 0;
  }
  line_t* n_line = psp_editor_add_line(n_line_id, "");

  if (col_id >= a_line->width) {
    /* nothing more to do */
  } else {
    int delta = a_line->width - col_id;
    psp_editor_resize_line(n_line, 1 + ((delta * 3) / 2));
    int index = col_id;
    while (index < a_line->width) {
      n_line->text[n_line->width++] = a_line->text[index];
      index++;
    }
    n_line->text[n_line->width] = 0;
    a_line->width = col_id;
  }
  g_text->num_lines++;
}

void
psp_editor_split_curr_line()
{
  psp_editor_split_line(g_text->curr_line, g_text->curr_col);
  g_text->curr_col = 0;
  g_text->curr_line++;
  psp_editor_rewrap_if_needed();
  psp_editor_update_column(1);
  psp_editor_update_line();
  psp_editor_update_sel();
  g_text->sel_mode = 0;
}

void
psp_editor_delete_char(int col_id)
{
  PSPWRITE.is_modified = 1;

  line_t* a_line = g_text->lines[g_text->curr_line];
  if (! a_line) {
    a_line = psp_editor_add_line(g_text->curr_line, "");
  }
  if (col_id > a_line->width) {
    /* Nothing to do */
  } else 
  if (col_id > 0) {
    int delta = a_line->width - col_id;
    if (delta > 0) {
      char* scan_text = &a_line->text[col_id];
      memcpy( scan_text - 1, scan_text, delta);
    }
    a_line->width--;
    a_line->text[a_line->width] = 0;
  }
}

void
psp_editor_suppr_char(int col_id)
{
  PSPWRITE.is_modified = 1;

  line_t* a_line = g_text->lines[g_text->curr_line];
  if (! a_line) {
    a_line = psp_editor_add_line(g_text->curr_line, "");
  }
  if (col_id > a_line->width) {
    /* Nothing to do */
  } else {
    int delta = a_line->width - col_id;
    if (delta > 0) {
      char* scan_text = &a_line->text[col_id];
      memcpy( scan_text, scan_text + 1, delta);
    }
    a_line->width--;
    a_line->text[a_line->width] = 0;
  }
  g_text->sel_mode = 0;
}

void
psp_editor_delete_line(int line_id)
{
  PSPWRITE.is_modified = 1;

  line_t* a_line = g_text->lines[line_id];
  if (a_line) {
    if (a_line->text) free(a_line->text);
    a_line->text = 0;
    free(a_line);
    g_text->lines[line_id] = 0;
    a_line = 0;
  }
  int delta = g_text->num_lines - line_id - 1;
  if (delta > 0) {
    memcpy(&g_text->lines[line_id], &g_text->lines[line_id +1], sizeof(line_t *) * delta);
    g_text->lines[g_text->num_lines - 1] = 0;
  }
  g_text->num_lines--;
  if (g_text->num_lines <= 0) {
    psp_editor_add_line(0, "");
  }
  g_text->sel_mode = 0;
}

void
psp_editor_join_prev_line(int a_line_id)
{
  /* Join with previous line */

  int p_line_id = a_line_id - 1;
  line_t* a_line = g_text->lines[a_line_id];
  line_t* p_line = g_text->lines[p_line_id];
  if (! a_line) {
    a_line = psp_editor_add_line(a_line_id, "");
  }
  if (! p_line) {
    p_line = psp_editor_add_line(p_line_id, "");
  }
  int a_width = a_line->width;
  int p_width = p_line->width;
  int r_width = p_width + a_width;
  psp_editor_resize_line(p_line, r_width);
  if (r_width > p_line->max_width) r_width = p_line->max_width;
  int delta = r_width - p_width;
  if (delta > 0) {
    memcpy(&p_line->text[p_width], a_line->text, delta);
  }
  p_line->width = r_width;
  p_line->text[p_line->width] = 0;
  g_text->curr_col  = p_width;
  g_text->curr_line = p_line_id;
  psp_editor_delete_line(a_line_id);
  g_text->sel_mode = 0;
}

void
psp_editor_delete_curr_char()
{
  if (g_text->sel_mode) {
    psp_editor_cut();
  } else {
    if (g_text->curr_col > 0) {
      psp_editor_delete_char(g_text->curr_col);
      g_text->curr_col--;
      psp_editor_rewrap_if_needed();
      psp_editor_update_column(1);
    } else 
    if (g_text->curr_line > 0) {
      psp_editor_join_prev_line(g_text->curr_line);
      psp_editor_rewrap_if_needed();
      psp_editor_update_column(1);
      psp_editor_update_line();
    }
  }
  g_text->sel_mode = 0;
}

void
psp_editor_suppr_curr_char()
{
  if (g_text->sel_mode) {
    psp_editor_cut();
  } else {
    line_t* a_line = g_text->lines[g_text->curr_line];
    if (! a_line) {
      a_line = psp_editor_add_line(g_text->curr_line, "");
    }
    if (g_text->curr_col >= a_line->width) {
      psp_editor_join_prev_line(g_text->curr_line + 1);
    } else {
      psp_editor_suppr_char(g_text->curr_col);
    }
  }
  g_text->sel_mode = 0;
  psp_editor_rewrap_if_needed();
}

void
psp_editor_clear_curr_line()
{
  int line_id = g_text->curr_line;
  line_t* a_line = g_text->lines[line_id];
  if (! a_line) {
    a_line = psp_editor_add_line(line_id, "");
  }
  a_line->width = 0;
  a_line->text[0] = 0;
  g_text->curr_col = 0;
  psp_editor_update_column(1);
  g_text->sel_mode = 0;
}

/* Recent file list */
# if 0 
void
psp_editor_display_recent()
{
  int index;
  recent_file_t* recent_file;

  for(index = 0; index < MAX_RECENT_FILE; index++) {
    recent_file = &PSPWRITE.recent_file[index];
    fprintf(stdout, "[%d] : %s %d %d %d %d\n",
      index, recent_file->filename, 
      recent_file->top_x, 
      recent_file->top_y, 
      recent_file->pos_x, 
      recent_file->pos_y );
  }
}
# endif

void
psp_editor_recent_save()
{
  recent_file_t* recent_file;
  struct stat    aStat;
  char*          new_filename;
  int            index;

  new_filename = PSPWRITE.edit_filename;

  recent_file  = &PSPWRITE.recent_file[0];
  /* If the new filename is not already in first position */
  if (strcasecmp(recent_file->filename, new_filename)) {

    /* First try to find the new filename in recent list */
    index = 1;
    while (index < MAX_RECENT_FILE) {
      recent_file = &PSPWRITE.recent_file[index];
      if (!strcasecmp(recent_file->filename, new_filename)) {
        int index_move = index;
        /* Found filename -> remove it */
        while (index_move < (MAX_RECENT_FILE-1)) {
          PSPWRITE.recent_file[index_move] = PSPWRITE.recent_file[index_move+1];
          index_move++;
        }
      }
      index++;
    }

    /* if file doesn't exist just return ... */
    if (stat(new_filename, &aStat)) {
      return;
    }

    /* Move down recent filename list */
    index = MAX_RECENT_FILE - 1;
    while (index > 0) {
      PSPWRITE.recent_file[index] = PSPWRITE.recent_file[index-1];
      index--;
    }
    recent_file = &PSPWRITE.recent_file[0];
    strcpy(recent_file->filename, PSPWRITE.edit_filename);
  }
  recent_file->top_x = g_text->left_col;
  recent_file->top_y = g_text->top_line;
  recent_file->pos_x = g_text->curr_col;
  recent_file->pos_y = g_text->curr_line;
}

void
psp_editor_recent_load()
{
  recent_file_t* recent_file;
  char*          filename;
  int            index;

  filename = PSPWRITE.edit_filename;
  for (index = 0; index < MAX_RECENT_FILE; index++) {
    recent_file = &PSPWRITE.recent_file[index];
    if (!strcasecmp(recent_file->filename, filename)) {
      /* Found filename */
      g_text->left_col  = recent_file->top_x;
      g_text->top_line  = recent_file->top_y;
      g_text->curr_col  = recent_file->pos_x;
      g_text->curr_line = recent_file->pos_y;

      psp_editor_update_column(1);
      psp_editor_update_line();
      psp_editor_recent_save();

      return;
    }
  }
}

void
psp_editor_new()
{
  psp_editor_reset_text();
  PSPWRITE.is_modified = 0;
  PSPWRITE.ask_overwrite = 1;

  /* Find a new valid filename */
  int index = 0;
  while (index < 1000) {
    if (index) {
      sprintf(PSPWRITE.edit_filename, "%snewfile_%03d.txt", PSPWRITE.psp_txtdir, index);
    } else {
      strcpy(PSPWRITE.edit_filename, PSPWRITE.psp_txtdir);
      strcat(PSPWRITE.edit_filename, "newfile.txt");
    }
    FILE* FileDesc = fopen(PSPWRITE.edit_filename, "r");
    if (! FileDesc) break;
    fclose(FileDesc);
    index++;
  }
}

static char loc_Buffer[EDITOR_MAX_WIDTH];
static char loc_Line[EDITOR_MAX_WIDTH + 32];

int
psp_editor_load(char *filename)
{
  FILE* FileDesc;

  psp_editor_recent_save();

  psp_editor_reset_text();
  PSPWRITE.is_modified = 0;

  FileDesc = fopen(filename, "r");
  if (FileDesc != (FILE *)0) {

    strncpy(PSPWRITE.edit_filename, filename, MAX_PATH);

    int line_id = 0;
    while (fgets(loc_Buffer, g_text->max_width, FileDesc) != (char *)0) {
      char *Scan = strchr(loc_Buffer,'\n');
      if (Scan) *Scan = '\0';
      /* For this #@$% of windows ! */
      Scan = strchr(loc_Buffer,'\r');
      if (Scan) *Scan = '\0';

      /* Copy the buffer into line */ 
      int tab;
      int index;
      int target = 0;
      for (index = 0; loc_Buffer[index]; index++) {
        uchar c = (uchar)loc_Buffer[index];
        /* expand tabs */
        if (c == '\t') {
          if (PSPWRITE.expand_tab) {
            for (tab = 0; tab < PSPWRITE.tab_stop; tab++) {
              loc_Line[target++] = ' ';
            }
          } else loc_Line[target++] = c;

        } else {

          if (c < ' ') c = ' ';
          loc_Line[target++] = c;
        }
        if (target >= EDITOR_MAX_WIDTH) break;
      }
      loc_Line[target] = 0;

      if ((target >= EDITOR_MAX_WIDTH) || (PSPWRITE.wrap_mode && target)) {
        if (g_text->wrap_buffer_size < target) {
          g_text->wrap_buffer = realloc(g_text->wrap_buffer, target + 1);
          g_text->wrap_buffer_size = target;
        }
        strcpy(g_text->wrap_buffer, loc_Line);
        line_id = psp_editor_rewrap_buffer_line(line_id, -1);
        
      } else {
        psp_editor_add_line(line_id, loc_Line);
        line_id++;
      }
      if (line_id >= g_text->max_lines) break;
    }
    PSPWRITE.is_modified = 0;
    fclose(FileDesc);

    psp_syntax_from_filename( PSPWRITE.edit_filename);
    psp_editor_recent_load();

    return 0;
  }
  return 1;
}

int
psp_editor_save()
{
  FILE* FileDesc;

  PSPWRITE.is_modified = 0;

  /* check if extention is present */
# if 0
  if (PSPWRITE.dos_mode) {
    char* filename = PSPWRITE.edit_filename;
    int length = strlen(filename);
    char* last_dot = strrchr(filename, '.');
    if (! last_dot || ((last_dot - filename) < (length - 4))  ) {
      if (length < (MAX_PATH-4)) {
        strcat(filename, ".txt");
      }
    }
  }
# endif
  /* check if fullpath ? */
  {
    char* found_slash = strchr(PSPWRITE.edit_filename, '/');
    if (! found_slash) {
      char TmpFileName[MAX_PATH];
      strcpy(TmpFileName, PSPWRITE.psp_txtdir);
      strcat(TmpFileName, PSPWRITE.edit_filename);
      strcpy(PSPWRITE.edit_filename, TmpFileName);
    }
  }

  FileDesc = fopen(PSPWRITE.edit_filename, "w");

  if (FileDesc != (FILE *)0) {

    int line_id = 0;
    int last_line_empty = 1;
    for (line_id = 0; line_id < g_text->num_lines; line_id++) {
      line_t* a_line = g_text->lines[line_id];
      if (a_line && a_line->width) { 
        memcpy(loc_Buffer, a_line->text, a_line->width);
        loc_Buffer[a_line->width] = 0;
        fputs(loc_Buffer, FileDesc);
        last_line_empty = 0;
      } else last_line_empty = 1;

      if (a_line && psp_editor_is_neol(a_line)) {
        fprintf(FileDesc, " ");
      } else {
        if (PSPWRITE.dos_mode) {
          fprintf(FileDesc, "\r");
        }
        fprintf(FileDesc, "\n");
      }
    }
    if (last_line_empty) {
      if (PSPWRITE.dos_mode) {
        fprintf(FileDesc, "\r");
      }
      fprintf(FileDesc, "\n");
    }
    fclose(FileDesc);

    psp_editor_recent_save();

    return 0;
  }
  return 1;
}

void 
psp_editor_init()
{
  if (! g_text) {
    g_text = (text_t *)malloc(sizeof( text_t ));
  }
  memset(g_text, 0, sizeof( text_t ));

  if (! g_clip) {
    g_clip = (clip_t *)malloc(sizeof( clip_t ));
  }
  memset(g_clip, 0, sizeof( clip_t ));

  g_text->max_width = EDITOR_MAX_WIDTH;
  g_text->max_lines = EDITOR_MAX_LINES;
  g_text->wrap_buffer = malloc( EDITOR_MAX_WIDTH + 1 );
  g_text->wrap_buffer_size = EDITOR_MAX_WIDTH;

  psp_editor_new();

  /* initialize color */
  editor_colors[COLOR_WHITE         ] = psp_sdl_rgb(255, 255, 255);
  editor_colors[COLOR_BLACK         ] = psp_sdl_rgb(0, 0, 0);
  editor_colors[COLOR_DARK_BLUE     ] = psp_sdl_rgb(0, 0, 0x55);
  editor_colors[COLOR_GREEN         ] = psp_sdl_rgb(0, 0xAA, 0);
  editor_colors[COLOR_RED           ] = psp_sdl_rgb(0xAA, 0, 0);
  editor_colors[COLOR_BROWN         ] = psp_sdl_rgb(0xAA, 0x55, 0);
  editor_colors[COLOR_MAGENTA       ] = psp_sdl_rgb(0xAA, 0, 0xAA);
  editor_colors[COLOR_ORANGE        ] = psp_sdl_rgb(255, 0xAA, 0);
  editor_colors[COLOR_YELLOW        ] = psp_sdl_rgb(255, 255, 0x55);
  editor_colors[COLOR_BRIGHT_GREEN  ] = psp_sdl_rgb(0, 255, 0);
  editor_colors[COLOR_CYAN          ] = psp_sdl_rgb(0, 0xAA, 0xAA);
  editor_colors[COLOR_BRIGHT_BLUE   ] = psp_sdl_rgb(0x55, 0x55, 255);
  editor_colors[COLOR_BLUE          ] = psp_sdl_rgb(0, 0, 0xAA);
  editor_colors[COLOR_PINK          ] = psp_sdl_rgb(255, 0, 255);
  editor_colors[COLOR_GRAY          ] = psp_sdl_rgb(0x55, 0x55, 0x55);
  editor_colors[COLOR_BRIGHT_GRAY   ] = psp_sdl_rgb(0xAA, 0xAA, 0xAA);
  editor_colors[COLOR_IMAGE         ] = PSP_MENU_TEXT_COLOR;

  psp_sdl_select_font( 0 );
}

# define PSP_EDITOR_BLINK_TIME 500

void
psp_display_cursor()
{
  psp_sdl_select_font( PSPWRITE.psp_font_size );

  int real_y_min = (PSP_SDL_EDITOR_HEIGHT - (PSPWRITE.screen_h * psp_font_height)) / 2;
  int real_x_min = (PSP_SDL_EDITOR_WIDTH  - (PSPWRITE.screen_w * psp_font_width)) / 2;

  /* Display cursor */
  int fg_color = editor_colors[PSPWRITE.fg_color];
  u32 curr_clock = SDL_GetTicks();
  if ((curr_clock - g_text->last_blink) > PSP_EDITOR_BLINK_TIME) {
    g_text->last_blink = curr_clock;
    g_text->cursor_blink = ! g_text->cursor_blink;
  }
  if (g_text->cursor_blink || g_text->cursor_moving) {
    int c_y = g_text->curr_line - g_text->top_line;
    int c_x = g_text->curr_col - g_text->left_col;
    if ((c_y >= 0) && (c_x >= 0)) {
      int c_real_y = real_y_min + c_y * psp_font_height;
      int c_real_x = real_x_min + c_x * psp_font_width;
      if (command_mode) {
        psp_sdl_fill_rectangle(c_real_x, c_real_y + psp_font_height, psp_font_width, 0, fg_color, 0);
      } else {
        psp_sdl_fill_rectangle(c_real_x, c_real_y, 0, psp_font_height, fg_color, 0);
      }
    }
  }
}

static int
psp_editor_is_sel_region(int line_id, int col_id)
{
  if (! g_text->sel_mode) return 0;

  if ((g_text->sel_to_line != g_text->sel_from_line) ||
      (g_text->sel_to_col  != g_text->sel_from_col )) {

    if (g_text->sel_to_line == g_text->sel_from_line) {
      if ((line_id == g_text->sel_from_line) &&
          (col_id  >= g_text->sel_from_col ) &&
          (col_id  <= g_text->sel_to_col   )) {
        return 1;
      }

    } else {
      if ((line_id == g_text->sel_from_line) &&
          (col_id  >= g_text->sel_from_col )) {
        return 1;
      }
      if ((line_id == g_text->sel_to_line) &&
          (col_id  <= g_text->sel_to_col )) {
        return 1;
      }
      if ((line_id < g_text->sel_to_line  ) &&
          (line_id > g_text->sel_from_line)) {
        return 1;
      }
    }
  }
  return 0;
}

void
psp_editor_clear_clipboard()
{
  line_list_t *scan_list;
  line_list_t *del_list;

  scan_list = g_clip->head_list; 
  while (scan_list) {
    del_list  = scan_list;
    scan_list = scan_list->next;
    line_t* del_line = del_list->line;
    if (del_line) {
      if (del_line->text) {
        free(del_line->text);
      }
      free(del_line);
    }
    free(del_list);
  }
  g_clip->head_list = 0;
  g_clip->last_list = 0;
  g_clip->number_lines = 0;
}

void
psp_editor_append_clipboard(line_t* a_line, int col_from, int col_to)
{
  line_t*      a_clip_line;
  line_list_t* a_line_list;

  a_line_list = (line_list_t*)malloc( sizeof(line_list_t) );
  a_line_list->next = 0;

  g_clip->number_lines++;
  if (! g_clip->head_list) {
    g_clip->head_list = a_line_list;
    g_clip->last_list = a_line_list;
  } else {
    g_clip->last_list->next = a_line_list;
    g_clip->last_list = a_line_list;
  }

  a_clip_line = psp_editor_alloc_line();
  a_line_list->line = a_clip_line;

  if (a_line && a_line->width) {
    if (col_from < a_line->width) {
      if (col_to > a_line->width) col_to = a_line->width;
      int length = col_to - col_from + 1;
      a_clip_line->width     = length;
      a_clip_line->max_width = length;
      a_clip_line->text = malloc( length + 1 );
      memcpy(a_clip_line->text, a_line->text + col_from, length);
      a_clip_line->text[length] = 0;
    }
  }
}

# if 0
void
psp_editor_display_clipboard()
{
  line_list_t* scan_list;
  for (scan_list = g_clip->head_list; scan_list; scan_list = scan_list->next) {
    line_t* a_line = scan_list->line;
    if (!a_line || !a_line->text) fprintf(stdout, "<empty>\n");
    else          fprintf(stdout, "%s\n", a_line->text);
  }
}
# endif

int
psp_editor_is_valid_sel()
{
  if (! g_text->sel_mode) return 0;

  if ((g_text->sel_from_line != g_text->sel_to_line) ||
      (g_text->sel_from_col  != g_text->sel_to_col )) return 1;

  return 0;
}

void
psp_editor_copy()
{
  line_t* a_line;
  int     curr_line;

  if (! psp_editor_is_valid_sel()) return;

  /* Clear previous clip board */
  psp_editor_clear_clipboard();

  /* Copy lines to clipboard */
  if (g_text->sel_from_line == g_text->sel_to_line) {
    line_t* a_line = g_text->lines[g_text->sel_from_line];
    psp_editor_append_clipboard(a_line, g_text->sel_from_col, g_text->sel_to_col);
  } else {
    /* first line */
    a_line = g_text->lines[g_text->sel_from_line];
    if (a_line) {
      psp_editor_append_clipboard(a_line, g_text->sel_from_col, a_line->width);
    } else {
      psp_editor_append_clipboard(a_line, 0, 0);
    }
    /* middle lines */
    curr_line = g_text->sel_from_line + 1;
    while (curr_line < g_text->sel_to_line) {
      a_line = g_text->lines[curr_line];
      if (a_line) {
        psp_editor_append_clipboard(a_line, 0, a_line->width);
      } else {
        psp_editor_append_clipboard(a_line, 0, 0);
      }
      curr_line++;
    }
    /* last line */
    a_line = g_text->lines[curr_line];
    if (a_line) {
      psp_editor_append_clipboard(a_line, 0, g_text->sel_to_col);
    } else {
      psp_editor_append_clipboard(a_line, 0, 0);
    }
  }

# if 0
  psp_editor_display_clipboard();
# endif
  g_text->sel_mode = 0;
}

void
psp_editor_del_col_from_to(line_t* a_line, int col_from, int col_to)
{
  if ((! a_line) || (col_from > a_line->width)) return;

  PSPWRITE.is_modified = 1;

  if (col_to > a_line->width) col_to = a_line->width;
  char* a_text = a_line->text;
  int  index = 0;
  for (index = col_to + 1; index <= a_line->width; index++) {
    a_text[col_from++] = a_text[index];
  }
  a_text[col_from] = 0;
  a_line->width = col_from;
}

void
psp_editor_del_line_from(int line_id, int num_lines)
{
  PSPWRITE.is_modified = 1;

  int index = 0;
  while (index < num_lines) {
    line_t* a_line = g_text->lines[line_id + index];
    if (a_line) {
      if (a_line->text) free(a_line->text);
      a_line->text = 0;
      free(a_line);
      g_text->lines[line_id + index] = 0;
      a_line = 0;
    }
    index++;
  }
  int line_id_last = line_id + num_lines;
  int delta = g_text->num_lines - line_id_last;
  if (delta > 0) {
    memcpy(&g_text->lines[line_id], &g_text->lines[line_id_last], sizeof(line_t *) * delta);
    memset(&g_text->lines[g_text->num_lines - num_lines], 0, sizeof(line_t *) * num_lines);
    g_text->num_lines -= num_lines;
  }
  if (g_text->num_lines <= 0) {
    psp_editor_add_line(0, "");
  }
  g_text->sel_mode = 0;
}

void
psp_editor_cut()
{
  if (! psp_editor_is_valid_sel()) return;

  psp_editor_copy();

  g_text->curr_col  = g_text->sel_from_col;
  g_text->curr_line = g_text->sel_from_line;

  line_t* a_line;

  /* only one line */
  if (g_text->sel_from_line == g_text->sel_to_line) {
    /* del characters between from -> to */
    a_line = g_text->lines[g_text->sel_from_line];
    psp_editor_del_col_from_to(a_line, g_text->sel_from_col, g_text->sel_to_col);

  } else {
    /* first line, del characters between from -> width */
    a_line = g_text->lines[g_text->sel_from_line];
    psp_editor_del_col_from_to(a_line, g_text->sel_from_col, a_line->width);
    /* last line */
    a_line = g_text->lines[g_text->sel_to_line];
    psp_editor_del_col_from_to(a_line, 0, g_text->sel_to_col);

    int delta = g_text->sel_to_line - g_text->sel_from_line - 1;
    if (delta > 0) {
      psp_editor_del_line_from(g_text->sel_from_line + 1, delta);
    }
    psp_editor_suppr_curr_char();
  }
  g_text->sel_mode = 0;
  psp_editor_rewrap_if_needed();
  psp_editor_update_column(1);
  psp_editor_update_line();
}

void
psp_editor_paste()
{
  if ((! g_clip->head_list) || (g_text->sel_mode)) return;

  /* merge first clip line with current line */
  line_list_t *scan_list = g_clip->head_list;
  if (scan_list) {
    line_t* a_line = scan_list->line;
    int index;
    for (index = 0; index < a_line->width; index++) {
      psp_editor_insert_char(g_text->curr_line, g_text->curr_col, a_line->text[index]);
      g_text->curr_col++;
    }
    scan_list = scan_list->next;
    if (scan_list) {
      psp_editor_split_curr_line();
    }
  }
  if (scan_list) {
    /* copy all other lines */
    while (scan_list != g_clip->last_list) {
      line_t* a_line = scan_list->line;
      char*   a_text = a_line->text;
      if (! a_text) a_text = "";
      psp_editor_insert_line(g_text->curr_line, a_text);
      g_text->curr_line++;
      scan_list = scan_list->next;
    }
  }
  /* last line */
  if (g_clip->head_list != g_clip->last_list) {
    line_t* a_line = g_clip->last_list->line;
    int index;
    for (index = 0; index < a_line->width; index++) {
      psp_editor_insert_char(g_text->curr_line, g_text->curr_col, a_line->text[index]);
      g_text->curr_col++;
    }
  }
  psp_editor_rewrap_if_needed();
  psp_editor_update_column(1);
  psp_editor_update_line();
}

void
psp_display_editor()
{
  char status_line[128];

  psp_sdl_select_font( PSPWRITE.psp_font_size );

  int line_from = g_text->top_line;
  int line_to   = line_from + PSPWRITE.screen_h;
  if (line_to > g_text->num_lines) line_to = g_text->num_lines;

  int line_id;

  int real_y_min = (PSP_SDL_EDITOR_HEIGHT - (PSPWRITE.screen_h * psp_font_height)) / 2;
  int real_x_min = (PSP_SDL_EDITOR_WIDTH  - (PSPWRITE.screen_w * psp_font_width )) / 2;


  int y = 0;
  int real_y = real_y_min;
  int fg_color = editor_colors[PSPWRITE.fg_color];

  if (PSPWRITE.bg_color == COLOR_IMAGE) {
    psp_sdl_blit_editor();
  } else {
    int bg_color = editor_colors[PSPWRITE.bg_color];
    psp_sdl_clear_screen(bg_color);
  }

  for (line_id = line_from; line_id < line_to; line_id++) {
    line_t* a_line = g_text->lines[line_id];
    if (a_line && a_line->text) {
      int col_from = g_text->left_col;
      int col_to   = a_line->width;
      int col_id;
      int x = 0;
      int real_x = real_x_min;
      int syn_len = 0;
      int syn_color = 0;
      int beg_word = 1;
      syntax_elem_t* syn_elem = 0;

      for (col_id = col_from; col_id < col_to; col_id++) {
        unsigned char c = a_line->text[col_id];
        if (beg_word) {
          syn_elem = psp_syntax_search_elem( a_line->text + col_id );
          if (syn_elem) {
            syn_len   = syn_elem->length;
            syn_color = editor_colors[syn_elem->color];
          }
        }
        if (c == '\t') c = 1;
        else if (c < ' ') c = ' ';

        beg_word = psp_syntax_is_delim( c );

        if (psp_editor_is_sel_region(line_id, col_id)) 
        {
          psp_sdl_put_char(real_x, real_y, fg_color, 0, c, 0, 1);
        } else 
        if (syn_elem && (! beg_word)) {
          psp_sdl_put_char(real_x, real_y, syn_color, 0, c, 1, 0);
        } else {
          psp_sdl_put_char(real_x, real_y, fg_color, 0, c, 1, 0);
        }
        real_x += psp_font_width;
        x++;
        if (x >= PSPWRITE.screen_w) break;

        if (syn_len > 0) syn_len--;
      }
    }
    real_y += psp_font_height;
    y++;
    if (y > PSPWRITE.screen_h) break;
  }

  psp_sdl_select_font( 0 );

  int real_y_status = PSP_SDL_EDITOR_HEIGHT;
  int per_cent = 0;
  if (g_text->num_lines) {
    per_cent = 100 * (g_text->curr_line + 1) / g_text->num_lines;
  }
  sprintf(status_line, "%d,%d   %d%%", 
          g_text->curr_line + 1, g_text->curr_col + 1, per_cent);
  int len = strlen(status_line);
  int real_x_status = (PSP_SDL_EDITOR_WIDTH - (len + 2) * psp_font_width);
  psp_sdl_print( real_x_status, real_y_status, status_line, fg_color );
 
  if (command_result[0]) {
    real_x_status = 0;
    psp_sdl_print( real_x_status, real_y_status, command_result, PSP_MENU_NOTE_COLOR );
  } else
  if (command_mode) {
    sprintf(status_line, ":%s", command_line);
    psp_sdl_print( 0, real_y_status, status_line, fg_color );
    if (g_text->cursor_blink) {
      real_x_status = (command_pos + 1) * psp_font_width;
      psp_sdl_fill_rectangle(real_x_status, real_y_status, 0, psp_font_height, fg_color, 0);
    }
  }

}

# define PSP_EDITOR_MIN_MOVE_TIME   10000
# define PSP_EDITOR_REPEAT_TIME    300000
# define PSP_EDITOR_MIN_EDIT_TIME  180000
# define PSP_EDITOR_MIN_TIME       150000

# define ANALOG_THRESHOLD 60

void 
psp_editor_get_analog_direction(int Analog_x, int Analog_y, int *x, int *y)
{
  int DeltaX = 255;
  int DeltaY = 255;
  int DirX   = 0;
  int DirY   = 0;

  *x = 0;
  *y = 0;

  if (Analog_x <=        ANALOG_THRESHOLD)  { DeltaX = Analog_x; DirX = -1; }
  else 
  if (Analog_x >= (255 - ANALOG_THRESHOLD)) { DeltaX = 255 - Analog_x; DirX = 1; }

  if (Analog_y <=        ANALOG_THRESHOLD)  { DeltaY = Analog_y; DirY = -1; }
  else 
  if (Analog_y >= (255 - ANALOG_THRESHOLD)) { DeltaY = 255 - Analog_y; DirY = 1; }

  *x = DirX;
  *y = DirY;
}

static void
psp_editor_edit_loop()
{
  int  danzeff_mode;
  int  danzeff_key;
  int  irkeyb_key;
  long old_pad;
  long new_pad;
  int  last_time;
  int  repeat_mode;
  int  new_Lx;
  int  new_Ly;

  SceCtrlData c;

  new_Lx       = 0;
  new_Ly       = 0;
  old_pad      = 0;
  last_time    = 0;
  danzeff_mode = 0;
  repeat_mode  = 0;

  new_pad = 0;

  while (! command_mode) {

    psp_display_editor();
    psp_display_cursor();

    if (danzeff_mode) {
      danzeff_moveTo(0, 0);
      danzeff_render();
    }
    psp_sdl_flip();

    while (1) {

      myCtrlPeekBufferPositive(&c, 1);
      c.Buttons &= PSP_ALL_BUTTON_MASK;

      if (! danzeff_mode) {
        psp_editor_get_analog_direction( c.Lx, c.Ly, &new_Lx, &new_Ly);

        if (new_Lx > 0) {
          psp_editor_goto_col_right();
        } else 
        if (new_Lx < 0) {
          psp_editor_goto_col_left();
        }
        if (new_Ly < 0) {
          psp_editor_goto_line_up();
        } else 
        if (new_Ly > 0) {
          psp_editor_goto_line_down();
        }

        if (new_Lx || new_Ly) {
          g_text->cursor_moving = 1;
          break;
        }
      }

# ifdef USE_PSP_IRKEYB
      irkeyb_key = psp_irkeyb_set_psp_key(&c);
# endif
      new_pad = c.Buttons;

      if (old_pad != new_pad) {
        repeat_mode = 0;
        last_time = c.TimeStamp;
        old_pad = new_pad;
        break;

      } else 
      if (new_pad != 0) {
        if ((c.TimeStamp - last_time) > PSP_EDITOR_REPEAT_TIME) {
          repeat_mode = 1;
        }
      } else {
        repeat_mode = 0;
      }

      if (repeat_mode) {
        if ((c.TimeStamp - last_time) > PSP_EDITOR_MIN_MOVE_TIME) {
          last_time = c.TimeStamp;
          old_pad = new_pad;
          break;
        }
      } else {

        if ((c.TimeStamp - last_time) > PSP_EDITOR_MIN_TIME) {
          last_time = c.TimeStamp;
          old_pad = new_pad;
          break;
        }
      }

# ifdef USE_PSP_IRKEYB
      irkeyb_key = psp_irkeyb_read_key();

      if (irkeyb_key > 0) {
        if (irkeyb_key == PSP_IRKEYB_SUPPR) {
          psp_editor_suppr_curr_char();
        } else 
        if (irkeyb_key == PSP_IRKEYB_INSERT) {
          /* nothing to do */
        } else 
        if (irkeyb_key == PSP_IRKEYB_HOME) {
          psp_editor_goto_first_line();
        } else 
        if (irkeyb_key == PSP_IRKEYB_PAGEUP) {
          psp_editor_goto_page_up();
        } else 
        if (irkeyb_key == PSP_IRKEYB_PAGEDOWN) {
          psp_editor_goto_page_down();
        } else 
        if (irkeyb_key == PSP_IRKEYB_END) {
          psp_editor_goto_last_line();
        } else 
        if (irkeyb_key >= ' ') {
          if (! PSPWRITE.is_read_only) psp_editor_insert_curr_char(irkeyb_key);
        } else
        if (irkeyb_key == 0x8) {
          if (! PSPWRITE.is_read_only) psp_editor_delete_curr_char();
        } else
        if (irkeyb_key == 0x9) {
          if (! PSPWRITE.is_read_only) psp_editor_insert_curr_char(irkeyb_key);
        } else
        if (irkeyb_key == 0xd) {
          if (! PSPWRITE.is_read_only) psp_editor_split_curr_line();
        } else /* ctrl L */
        if (irkeyb_key == 0xc) {
          if (! PSPWRITE.is_read_only) psp_editor_clear_curr_line();
        } else /* ctrl C */
        if (irkeyb_key == 0x3) {
          psp_editor_copy();
        } else /* ctrl V */
        if (irkeyb_key == 0x16) {
          psp_editor_sel_mode();
        } else /* ctrl D */
        if (irkeyb_key == 0x4) {
          if (! PSPWRITE.is_read_only) psp_editor_cut();
        } else /* ctrl B */
        if (irkeyb_key == 0x2) {
          psp_editor_goto_word_left();
        } else /* ctrl N */
        if (irkeyb_key == 0xe) {
          psp_editor_goto_word_right();
        } else /* ctrl P */
        if (irkeyb_key == 0x10) {
          if (! PSPWRITE.is_read_only) psp_editor_paste();
        } else /* Escape */
        if (irkeyb_key == 0x1b) {
          command_mode = 1;
        }
        
        last_time = c.TimeStamp;
        break;
      }
# endif
    }

    if (danzeff_mode) {

      danzeff_key = danzeff_readInput(c);

      if (danzeff_key > DANZEFF_START) {

        if (danzeff_key >= ' ') {
         if (! PSPWRITE.is_read_only)  psp_editor_insert_curr_char(danzeff_key);
        } else
        if (danzeff_key == DANZEFF_DEL) {
          if (! PSPWRITE.is_read_only) psp_editor_delete_curr_char();
        } else
        if (danzeff_key == DANZEFF_TAB) {
          if (! PSPWRITE.is_read_only) psp_editor_insert_curr_char('\t');
        } else
        if (danzeff_key == DANZEFF_CLEAR) {
          if (! PSPWRITE.is_read_only) psp_editor_clear_curr_line();
        } else
        if (danzeff_key == DANZEFF_PAGE_UP) {
          psp_editor_goto_page_up();
        } else
        if (danzeff_key == DANZEFF_PAGE_DOWN) {
          psp_editor_goto_page_down();
        } else
        if (danzeff_key == DANZEFF_END) {
          psp_editor_goto_col_end();
        } else
        if (danzeff_key == DANZEFF_HOME) {
          psp_editor_goto_first_line();
        } else
        if (danzeff_key == DANZEFF_ENTER) {
          if (! PSPWRITE.is_read_only) psp_editor_split_curr_line();
        } else
        if (danzeff_key == DANZEFF_UP) {
          psp_editor_goto_col_begin();
        } else
        if (danzeff_key == DANZEFF_DOWN) {
          if (! PSPWRITE.is_read_only) psp_editor_split_curr_line();
        } else
        if (danzeff_key == DANZEFF_LEFT) {
          psp_editor_goto_col_left();
        } else
        if (danzeff_key == DANZEFF_RIGHT) {
          psp_editor_goto_col_right();
        }

      } else
      if ((danzeff_key == DANZEFF_START ) || 
          (danzeff_key == DANZEFF_SELECT)) 
      {
        danzeff_mode = 0;
        old_pad = new_pad = 0;

        psp_kbd_wait_no_button();
      }

      continue;
    }

    if (!c.Buttons) {
      g_text->cursor_moving = 0;
    }

    if ((c.Buttons & (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) ==
        (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) {
      /* Exit ! */
      psp_sdl_exit(0);
    } else
    if(new_pad & PSP_CTRL_START) {
      danzeff_mode = 1;
    } else
    if (new_pad & PSP_CTRL_RTRIGGER) {

      if(new_pad & PSP_CTRL_UP) {
        psp_editor_goto_page_up();
      } else
      if(new_pad & PSP_CTRL_DOWN) {
        psp_editor_goto_page_down();
      } else
      if(new_pad & PSP_CTRL_RIGHT) {
        psp_editor_goto_word_right();
      } else
      if(new_pad & PSP_CTRL_LEFT) {
        psp_editor_goto_word_left();
      } else
      if(new_pad & PSP_CTRL_SELECT) {
        psp_editor_sel_mode();
      } else
      if(new_pad & PSP_CTRL_TRIANGLE) {
        psp_editor_copy();
      } else
      if(new_pad & PSP_CTRL_CROSS) {
        if (! PSPWRITE.is_read_only) psp_editor_cut();
      } else
      if(new_pad & PSP_CTRL_CIRCLE) {
        if (! PSPWRITE.is_read_only) psp_editor_paste();
      } else
      if(new_pad & PSP_CTRL_SQUARE) {
        if (! PSPWRITE.is_read_only) psp_editor_rewrap_curr_line();
      }

    } else
    if (new_pad & PSP_CTRL_LTRIGGER) {

      if(new_pad & PSP_CTRL_UP) {
        psp_editor_goto_first_line();
      } else
      if(new_pad & PSP_CTRL_DOWN) {
        psp_editor_goto_last_line();
      } else
      if(new_pad & PSP_CTRL_RIGHT) {
        psp_editor_goto_col_end();
      } else
      if(new_pad & PSP_CTRL_LEFT) {
        psp_editor_goto_col_begin();
      } else
      if(new_pad & PSP_CTRL_SELECT) {
        command_mode = 1;
      } else
      if(new_pad & PSP_CTRL_TRIANGLE) {
        psp_editor_goto_first_line();
      } else
      if(new_pad & PSP_CTRL_CROSS) {
        psp_editor_goto_last_line();
      } else
      if(new_pad & PSP_CTRL_CIRCLE) {
        psp_editor_goto_col_end();
      } else
      if(new_pad & PSP_CTRL_SQUARE) {
        psp_editor_goto_col_begin();
      }

    } else
    if(new_pad & PSP_CTRL_UP) {
      psp_editor_goto_line_up();
    } else
    if(new_pad & PSP_CTRL_DOWN) {
      psp_editor_goto_line_down();
    } else
    if(new_pad & PSP_CTRL_RIGHT) {
      psp_editor_goto_col_right();
    } else
    if(new_pad & PSP_CTRL_LEFT) {
      psp_editor_goto_col_left();
    } else
    if(new_pad & PSP_CTRL_TRIANGLE) {
      if (! PSPWRITE.is_read_only) psp_editor_delete_curr_char();
    } else
    if(new_pad & PSP_CTRL_SQUARE) {
      if (! PSPWRITE.is_read_only) psp_editor_suppr_curr_char();
    } else
    if(new_pad & PSP_CTRL_CIRCLE) {
      if (! PSPWRITE.is_read_only) psp_editor_insert_curr_char(' ');
    } else
    if(new_pad & PSP_CTRL_CROSS) {
      if (! PSPWRITE.is_read_only) psp_editor_split_curr_line();
    } else
    if(new_pad & PSP_CTRL_SELECT) {
      psp_edit_main_menu();
    }
  }
}

static void
psp_command_insert_char( char c )
{
  if (command_pos < EDITOR_COMMAND_WIDTH) {
    command_line[command_pos++] = c;
    command_line[command_pos  ] = 0;
  }
}

static void
psp_command_delete_char()
{
  if (command_pos > 0) {
    command_line[--command_pos] = 0;
  }
}

static void
psp_command_history( int step )
{
  if (step > 0) {
    if (command_history_pos == 0) {
      strcpy(command_history[0], command_line);
    }
    if (command_history_pos < command_history_size) {
      command_history_pos++;
      strcpy(command_line, command_history[ command_history_pos ]);
      command_pos = strlen(command_line);
    }

  } else {
    if (command_history_pos > 0) {
      command_history_pos--;
      strcpy(command_line, command_history[ command_history_pos ]);
      command_pos = strlen(command_line);
    }

  }
}

static void
psp_command_list( int step )
{
  if (command_pos != 0) return;

  if (step > 0) {
    if (command_curr_list < EDITOR_MAX_COMMAND) command_curr_list++;
    else command_curr_list = 0;

  } else {
    if (command_curr_list > 0) command_curr_list--;
    else command_curr_list = EDITOR_MAX_COMMAND-1;
  }
  strcpy(command_line, command_list[ command_curr_list ]);
}

static void
psp_command_list_valid()
{
  if (command_pos != 0) return;

  strcpy(command_line, command_list[ command_curr_list ]);
  command_pos = strlen(command_line);
}

static void
psp_command_goto_line( char* line_num )
{
  int line_id;
  int end_scan;
  int index;
  int pattern_len;

  line_id = atoi(line_num) - 1;
  if ((line_id <= 0) || (line_id > g_text->num_lines)) {
    strcpy(command_result, "out of range");
    return;
  }
  g_text->cursor_moving = 1;
  g_text->curr_col  = 0;
  g_text->curr_line = line_id;
  psp_editor_update_column(0);
  psp_editor_update_line();

  command_mode = 0;
}


static char loc_search[EDITOR_COMMAND_WIDTH+1];

static int
psp_command_search_forward( char* pattern )
{
  int line_id;
  int beg_scan;
  int index;
  int pattern_len;

  if (! pattern[0]) {
    pattern = loc_search;
  } else {
    strcpy(loc_search, pattern);
  }
  pattern_len = strlen(pattern);
  if (! pattern_len) return 0;

  for (line_id = g_text->curr_line; line_id < g_text->num_lines; line_id++) {
    line_t* a_line = g_text->lines[line_id];
    if (!a_line || !a_line->width) continue;

    beg_scan = 0;
    if (line_id == g_text->curr_line) {
      if ((a_line->width - (g_text->curr_col+1)) <= 0) continue;
      beg_scan = g_text->curr_col+1;
    }
    for (index = beg_scan; index < a_line->width; index++) {
      if (! strncmp( a_line->text + index, pattern, pattern_len)) {
        g_text->curr_line = line_id;
        g_text->curr_col  = index;
        psp_editor_update_column(1);
        psp_editor_update_line();
        psp_editor_update_sel();
        return 1;
      }
    }
  }
  strcpy(command_result, "not found");
  return 0;
}

static void
psp_command_search_backward( char* pattern )
{
  int line_id;
  int end_scan;
  int index;
  int pattern_len;

  if (! pattern[0]) {
    pattern = loc_search;
  } else {
    strcpy(loc_search, pattern);
  }
  pattern_len = strlen(pattern);
  if (! pattern_len) return;

  for (line_id = g_text->curr_line; line_id >= 0; line_id--) {
    line_t* a_line = g_text->lines[line_id];
    if (!a_line || !a_line->width) continue;

    end_scan = a_line->width-1 ;
    if (line_id == g_text->curr_line) {
      if (g_text->curr_col < 1) continue;
      end_scan = g_text->curr_col-1;
    }
    for (index = end_scan; index >= 0; index--) {
      if (! strncmp( &a_line->text[ index ], pattern, pattern_len)) {
        g_text->curr_line = line_id;
        g_text->curr_col  = index;
        psp_editor_update_column(1);
        psp_editor_update_line();
        psp_editor_update_sel();
        return;
      }
    }
  }
  strcpy(command_result, "not found");
}

static void
psp_command_execute( char* command_line )
{
  char c = command_line[0];
  command_result[0] = 0;

  if (c == '/') {
    /* Search forward command */
    psp_command_search_forward( command_line + 1 );
  } else
  if (c == '?') {
    /* Search backward command */
    psp_command_search_backward( command_line + 1 );
  } else 
  if ((c >= '0') && (c <= '9')) {
    psp_command_goto_line( command_line );
  } else {
    snprintf(command_result, EDITOR_COMMAND_WIDTH, "unknown command '%s'", command_line);
  }
}

static void
psp_command_validate()
{
  int index;

  if (command_pos == 0) {
    if (command_history_size == 0) return;

    strcpy( command_line, command_history[1]);
    command_pos = strlen(command_line);
  }
  psp_command_execute( command_line );

  if ((command_history_size                == 0) ||
      (strcmp(command_history[1], command_line))) {

    if (command_history_size < EDITOR_COMMAND_HISTORY) {
      command_history_size++;
    }
    for (index = command_history_size; index > 1; index--) {
      strcpy( command_history[index], command_history[index-1]);
    }
    strcpy(command_history[1], command_line);
  }

  command_pos = 0;
  command_history[0][0] = 0;
  command_line[0] = 0;
  command_history_pos = 0;
}

static void
psp_command_clear()
{
  command_line[0] = 0;
  command_pos = 0;
}

static void
psp_command_loop()
{
  int  danzeff_mode;
  int  danzeff_key;
  int  irkeyb_key;
  long old_pad;
  long new_pad;
  int  last_time;
  int  repeat_mode;

  SceCtrlData c;

  old_pad      = 0;
  last_time    = 0;
  danzeff_mode = 0;
  repeat_mode  = 0;

  new_pad = 0;

  while (command_mode) {

    psp_display_editor();
    psp_display_cursor();

    if (danzeff_mode) {
      danzeff_moveTo(0, 0);
      danzeff_render();
    }
    psp_sdl_flip();

    if (command_result[0]) {
      sleep(1);
      command_result[0] = 0;
    }

    while (1) {

      myCtrlPeekBufferPositive(&c, 1);
      c.Buttons &= PSP_ALL_BUTTON_MASK;

# ifdef USE_PSP_IRKEYB
      irkeyb_key = psp_irkeyb_set_psp_key(&c);
# endif
      new_pad = c.Buttons;

      if (old_pad != new_pad) {
        repeat_mode = 0;
        last_time = c.TimeStamp;
        old_pad = new_pad;
        break;

      } else 
      if (new_pad != 0) {
        if ((c.TimeStamp - last_time) > PSP_EDITOR_REPEAT_TIME) {
          repeat_mode = 1;
        }
      } else {
        repeat_mode = 0;
      }

      if (repeat_mode) {
        if ((c.TimeStamp - last_time) > PSP_EDITOR_MIN_MOVE_TIME) {
          last_time = c.TimeStamp;
          old_pad = new_pad;
          break;
        }
      } else {

        if ((c.TimeStamp - last_time) > PSP_EDITOR_MIN_TIME) {
          last_time = c.TimeStamp;
          old_pad = new_pad;
          break;
        }
      }

# ifdef USE_PSP_IRKEYB
      irkeyb_key = psp_irkeyb_read_key();

      if (irkeyb_key > 0) {
        if (irkeyb_key == ' ') {
          if (command_pos == 0) psp_command_list_valid();
          else psp_command_insert_char(irkeyb_key);
        } else
        if (irkeyb_key > ' ') {
          psp_command_insert_char(irkeyb_key);
        } else
        if (irkeyb_key == 0x8) {
          psp_command_delete_char();
        } else
        if (irkeyb_key == 0x9) {
          psp_command_insert_char(irkeyb_key);
        } else
        if (irkeyb_key == 0xd) {
          psp_command_validate();
        } else /* ctrl L */
        if (irkeyb_key == 0xc) {
          psp_command_clear();
        } else /* Escape */
        if (irkeyb_key == 0x1b) {
          command_mode = 0;
        }
        
        last_time = c.TimeStamp;
        break;
      }
# endif
    }

    if (danzeff_mode) {

      danzeff_key = danzeff_readInput(c);

      if (danzeff_key > DANZEFF_START) {

        if (danzeff_key == DANZEFF_UP) {
          psp_command_history( -1 );
        } else
        if (danzeff_key == DANZEFF_DOWN) {
          psp_command_history( 1 );
        } else
        if (danzeff_key == DANZEFF_LEFT) {
          if (command_pos == 0) psp_command_list( -1 );
          else psp_command_validate();
        } else
        if (danzeff_key == DANZEFF_RIGHT) {
          if (command_pos == 0) psp_command_list( 1 );
          else psp_command_validate();
        } else
        if (danzeff_key == ' ') {
          if (command_pos == 0) psp_command_list_valid();
          else psp_command_insert_char(danzeff_key);
        } else
        if (danzeff_key > ' ') {
          psp_command_insert_char(danzeff_key);
        } else
        if (danzeff_key == DANZEFF_DEL) {
          psp_command_delete_char();
        } else
        if (danzeff_key == DANZEFF_CLEAR) {
          psp_command_clear();
        } else
        if (danzeff_key == DANZEFF_ENTER) {
          psp_command_validate();
        }

      } else
      if ((danzeff_key == DANZEFF_START ) || 
          (danzeff_key == DANZEFF_SELECT)) 
      {
        danzeff_mode = 0;
        old_pad = new_pad = 0;

        psp_kbd_wait_no_button();
      }

      continue;
    }

    if ((c.Buttons & (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) ==
        (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) {
      /* Exit ! */
      psp_sdl_exit(0);
    } else
    if(new_pad & PSP_CTRL_START) {
      danzeff_mode = 1;
    } else
    if (new_pad & PSP_CTRL_RTRIGGER) {

    } else
    if (new_pad & PSP_CTRL_LTRIGGER) {

      if(new_pad & PSP_CTRL_SELECT) {
        command_mode = 0;
      }

    } else
    if(new_pad & PSP_CTRL_TRIANGLE) {
      psp_command_delete_char();
    } else
    if(new_pad & PSP_CTRL_CIRCLE) {
      if (command_pos == 0) psp_command_list_valid();
      else psp_command_insert_char(' ');
    } else
    if(new_pad & PSP_CTRL_CROSS) {
      psp_command_validate();
    } else
    if(new_pad & PSP_CTRL_DOWN) {
      psp_command_history( -1 );
    } else
    if(new_pad & PSP_CTRL_UP) {
      psp_command_history(  1 );
    } else
    if(new_pad & PSP_CTRL_LEFT) {
      if (command_pos == 0) psp_command_list( -1 );
      else psp_command_validate();
    } else
    if(new_pad & PSP_CTRL_RIGHT) {
      if (command_pos == 0) psp_command_list(  1 );
      else psp_command_validate();
    } else
    if(new_pad & PSP_CTRL_SELECT) {
      psp_edit_main_menu();
    }
  }
}

void
psp_editor_main_loop()
{
  command_mode = 0;

  psp_editor_init();

  psp_edit_main_menu();

  while (1) {

    if (command_mode) {
      psp_command_loop();
    } else {
      psp_editor_edit_loop();
    }

    psp_kbd_wait_no_button();
  }
}
