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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspiofilemgr.h>
#include <psppower.h>

#include "psp_global.h"
#include "psp_danzeff.h"
#include "psp_sdl.h"
#include "psp_menu.h"
#include "psp_battery.h"
#include "psp_help.h"
#include "psp_irkeyb.h"
#include "psp_fmgr.h"
#include "psp_editor.h"
#include "psp_syntax.h"

# define MENU_SET_WIDTH        0
# define MENU_SET_HEIGHT       1
# define MENU_SET_WORD_WRAP    2
# define MENU_SET_WRAP_WIDTH   3
# define MENU_SET_FG_COLOR     4
# define MENU_SET_BG_COLOR     5
# define MENU_SET_FONT_SIZE    6
# define MENU_SET_SYNTAX       7
# define MENU_SET_DOS_MODE     8
# define MENU_SET_EXPAND_TAB   9
# define MENU_SET_TAB_SIZE    10
# define MENU_SET_MOVE_CURSOR 11
# define MENU_SET_CLOCK       12

# define MENU_SET_LOAD        13
# define MENU_SET_SAVE        14
# define MENU_SET_RESET       15

# define MENU_SET_BACK        16

# define MAX_MENU_SET_ITEM (MENU_SET_BACK + 1)

  static menu_item_t menu_list[] =
  {
    { "Width      : " },
    { "Height     : " },
    { "Word wrap  : " },
    { "Wrap width : " },
    { "Text color : " },
    { "Back color : " },
    { "Font size  : " },
    { "Syntax     : " },
    { "Dos mode   : " },
    { "Expand tab : " },
    { "Tab size   : " },
    { "Cursor     : " },
    { "CPU Clock  : " },

    { "Load settings" },
    { "Save settings" },
    { "Reset settings"},

    { "Back to Menu"  }
  };

  static int cur_menu_id = MENU_SET_SAVE;

  static int psp_cpu_clock = 222;
  static int psp_font_size = PSP_FONT_6X10;
  static int dos_mode = 0;
  static int expand_tab = 0;
  static int tab_stop = 0;
  static int move_on_text = 0;
  static int fg_color = 0;
  static int bg_color = 0;
  static int screen_w = 0;
  static int screen_h = 0;
  static int wrap_w    = 0;
  static int wrap_mode = 0;

  static int EDITOR_SCREEN_HEIGHT = 0;
  static int EDITOR_SCREEN_WIDTH  = 0;

static void 
psp_display_screen_settings_menu(void)
{
  char buffer[512];

  int menu_id = 0;
  int color   = 0;
  int x       = 0;
  int y       = 0;
  int y_step  = 0;

  {
    psp_sdl_blit_menu();

    psp_sdl_draw_rectangle(10,10,459,249,PSP_MENU_BORDER_COLOR,0);
    psp_sdl_draw_rectangle(11,11,457,247,PSP_MENU_BORDER_COLOR,0);

    psp_sdl_back_print(  30, 6, " Settings ", PSP_MENU_NOTE_COLOR);

    psp_display_screen_menu_battery();

    psp_sdl_back_print(30, 254, " []: Cancel  O/X: Valid  Sel: Back ", 
                       PSP_MENU_BORDER_COLOR);

    psp_sdl_back_print(370, 254, " By Zx-81 ",
                       PSP_MENU_AUTHOR_COLOR);
  }
  
  x      = 20;
  y      = 25;
  y_step = 10;
  
  for (menu_id = 0; menu_id < MAX_MENU_SET_ITEM; menu_id++) {
    color = PSP_MENU_TEXT_COLOR;
    if (cur_menu_id == menu_id) color = PSP_MENU_SEL_COLOR;

    psp_sdl_back_print(x, y, menu_list[menu_id].title, color);

    if (menu_id == MENU_SET_MOVE_CURSOR) {
      if (move_on_text) strcpy(buffer, "only on text");
      else              strcpy(buffer, "everywhere");
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(120, y, buffer, color);

    } else
    if (menu_id == MENU_SET_WIDTH) {
      sprintf(buffer, "%d", screen_w);
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(120, y, buffer, color);

    } else
    if (menu_id == MENU_SET_WRAP_WIDTH) {
      sprintf(buffer, "%d", wrap_w);
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(120, y, buffer, color);

    } else
    if (menu_id == MENU_SET_HEIGHT) {
      sprintf(buffer, "%d", screen_h);
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(120, y, buffer, color);

    } else
    if (menu_id == MENU_SET_FG_COLOR) {
      sprintf(buffer,"%s", editor_colors_name[fg_color]);
      color = editor_colors[fg_color];
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(120, y, buffer, color);

    } else
    if (menu_id == MENU_SET_BG_COLOR) {
      sprintf(buffer,"%s", editor_colors_name[bg_color]);
      color = editor_colors[bg_color];
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(120, y, buffer, color);

    } else
    if (menu_id == MENU_SET_FONT_SIZE) {
      strcpy(buffer, psp_all_fonts[ psp_font_size ].name );
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_SET_SYNTAX) {
      strcpy(buffer, psp_syntax_get_current());
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_SET_DOS_MODE) {
      if (dos_mode) strcpy(buffer, "yes");
      else                  strcpy(buffer, "no");
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_SET_WORD_WRAP) {
      if (wrap_mode) strcpy(buffer, "yes");
      else           strcpy(buffer, "no");
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_SET_EXPAND_TAB) {
      if (expand_tab) strcpy(buffer, "yes");
      else                     strcpy(buffer, "no");
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_SET_TAB_SIZE) {
      sprintf(buffer, "%d", tab_stop);
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(120, y, buffer, color);
    } else
    if (menu_id == MENU_SET_CLOCK) {
      sprintf(buffer, "%d", psp_cpu_clock);
      string_fill_with_space(buffer, 32);
      psp_sdl_back_print(120, y, buffer, color);
      y += y_step;

    } else
    if (menu_id == MENU_SET_RESET) {
      y += y_step;
    }

    y += y_step;
  }
}

static void
psp_settings_menu_validate(void)
{
  PSPWRITE.psp_cpu_clock = psp_cpu_clock;
  PSPWRITE.dos_mode      = dos_mode;
  PSPWRITE.expand_tab    = expand_tab;
  PSPWRITE.tab_stop      = tab_stop;
  PSPWRITE.move_on_text  = move_on_text;
  PSPWRITE.fg_color      = fg_color;
  PSPWRITE.bg_color      = bg_color;
  PSPWRITE.screen_h      = screen_h;
  PSPWRITE.screen_w      = screen_w;
  PSPWRITE.psp_font_size = psp_font_size;
  PSPWRITE.wrap_mode     = wrap_mode;
//TO_BE_DONE: rewrap ??
  PSPWRITE.wrap_w        = wrap_w;

  scePowerSetClockFrequency(PSPWRITE.psp_cpu_clock, PSPWRITE.psp_cpu_clock, PSPWRITE.psp_cpu_clock/2);
}

static void
psp_settings_menu_init()
{
  psp_cpu_clock = PSPWRITE.psp_cpu_clock;
  psp_font_size = PSPWRITE.psp_font_size;
  dos_mode      = PSPWRITE.dos_mode;
  wrap_mode     = PSPWRITE.wrap_mode;
  wrap_w        = PSPWRITE.wrap_w;
  expand_tab    = PSPWRITE.expand_tab;
  tab_stop      = PSPWRITE.tab_stop;
  move_on_text  = PSPWRITE.move_on_text;
  fg_color      = PSPWRITE.fg_color;
  bg_color      = PSPWRITE.bg_color;
  screen_h      = PSPWRITE.screen_h;
  screen_w      = PSPWRITE.screen_w;

  EDITOR_SCREEN_WIDTH  = PSP_SDL_EDITOR_WIDTH  / psp_all_fonts[ psp_font_size ].width;
  EDITOR_SCREEN_HEIGHT = PSP_SDL_EDITOR_HEIGHT / psp_all_fonts[ psp_font_size ].height;
}

void
psp_settings_set_font_size( int font_size )
{
  PSPWRITE.psp_font_size = font_size;
  EDITOR_SCREEN_WIDTH  = PSP_SDL_EDITOR_WIDTH  / psp_all_fonts[ font_size ].width;
  EDITOR_SCREEN_HEIGHT = PSP_SDL_EDITOR_HEIGHT / psp_all_fonts[ font_size ].height;

  if (PSPWRITE.screen_w > EDITOR_SCREEN_WIDTH) {
    PSPWRITE.screen_w = EDITOR_SCREEN_WIDTH;
  }
  if (PSPWRITE.screen_h > EDITOR_SCREEN_HEIGHT) {
    PSPWRITE.screen_h = EDITOR_SCREEN_HEIGHT;
  }
}

static void
psp_settings_menu_save_config()
{
  int error;

  psp_settings_menu_validate();

  error = psp_global_save_config();

  if (! error) /* save OK */
  {
    psp_display_screen_settings_menu();
    psp_sdl_back_print(270, 80, "Config file saved !", 
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  else 
  {
    psp_display_screen_settings_menu();
    psp_sdl_back_print(270, 80, "Can't save config file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
}

static void
psp_settings_menu_load_config()
{
  int error;

  error = psp_global_load_config();

  if (! error) /* load OK */
  {
    psp_display_screen_settings_menu();
    psp_sdl_back_print(270, 80, "Config file loaded !", 
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
    psp_settings_menu_init();
  }
  else 
  {
    psp_display_screen_settings_menu();
    psp_sdl_back_print(270, 80, "Can't load config file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
}

static void
psp_settings_menu_reset(void)
{
  psp_display_screen_settings_menu();
  psp_sdl_back_print(270, 80, "Reset settings !", 
                     PSP_MENU_WARNING_COLOR);
  psp_sdl_flip();
  psp_global_default();
  psp_settings_menu_init();
  sleep(1);
}

int
psp_settings_menu_confirm_overwrite()
{
  SceCtrlData c;

  if (PSPWRITE.ask_overwrite) {

    struct stat aStat;
    if (stat(PSPWRITE.edit_filename, &aStat)) {
      PSPWRITE.ask_overwrite = 0;
      return 1;
    }

    psp_display_screen_settings_menu();
    psp_sdl_back_print(200, 70, "X to overwrite ? [] to cancel", PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();

    psp_kbd_wait_no_button();
    do
    {
      myCtrlPeekBufferPositive(&c, 1);
      c.Buttons &= PSP_ALL_BUTTON_MASK;

# ifdef USE_PSP_IRKEYB
      psp_irkeyb_set_psp_key(&c);
# endif

      if (c.Buttons & PSP_CTRL_CROSS) {
        return 1;
      } else 
      if (c.Buttons & PSP_CTRL_SQUARE) {
        return 0;
      }

    } while (c.Buttons == 0);
    psp_kbd_wait_no_button();
  }
  return 0;
}

void
psp_settings_menu_load(int format)
{
  int ret;

  ret = psp_fmgr_menu(format);
  if (ret ==  1) /* load OK */
  {
    psp_display_screen_settings_menu();
    psp_sdl_back_print(270,  60, "File loaded !", PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
    psp_settings_menu_init();
  }
  else 
  if (ret == -1) /* Load Error */
  {
    psp_display_screen_settings_menu();
    psp_sdl_back_print(270,  60, "Can't load file !", PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
}

void
psp_settings_menu_color( int step )
{
  if (step < 0) {
    fg_color--;
    if (fg_color < 0) fg_color = COLOR_IMAGE-1;
  } else {
    fg_color++;
    if (fg_color >= COLOR_IMAGE) fg_color = 0;
  }
}

void
psp_settings_menu_font_size( int step ) 
{
  if (step < 0) {
    psp_font_size--;
    if (psp_font_size < 0) psp_font_size = PSP_SDL_MAX_FONT-1;
  } else {
    psp_font_size++;
    if (psp_font_size >= PSP_SDL_MAX_FONT) psp_font_size = 0;
  }

  EDITOR_SCREEN_WIDTH  = PSP_SDL_EDITOR_WIDTH  / psp_all_fonts[ psp_font_size ].width;
  EDITOR_SCREEN_HEIGHT = PSP_SDL_EDITOR_HEIGHT / psp_all_fonts[ psp_font_size ].height;

  screen_w = EDITOR_SCREEN_WIDTH;
  screen_h = EDITOR_SCREEN_HEIGHT;
}

void
psp_settings_menu_width( int step )
{
  if (step < 0) {
    screen_w--;
    if (screen_w < EDITOR_SCREEN_MIN_WIDTH) {
      screen_w = EDITOR_SCREEN_WIDTH;
    }
  } else {
    screen_w++;
    if (screen_w > EDITOR_SCREEN_WIDTH) {
      screen_w = EDITOR_SCREEN_MIN_WIDTH;
    }
  }
}

void
psp_settings_menu_wrap_width( int step )
{
  if (step < 0) {
    wrap_w--;
    if (wrap_w < EDITOR_SCREEN_MIN_WIDTH) {
      wrap_w = EDITOR_MAX_WRAP_WIDTH;
    }
  } else {
    wrap_w++;
    if (wrap_w > EDITOR_MAX_WRAP_WIDTH) {
      wrap_w = EDITOR_SCREEN_MIN_WIDTH;
    }
  }
}

#define MAX_CLOCK_VALUES 5
static int clock_values[MAX_CLOCK_VALUES] = { 133, 222, 266, 300, 333 };

static void
psp_settings_menu_clock(int step)
{
  int index;
  for (index = 0; index < MAX_CLOCK_VALUES; index++) {
    if (psp_cpu_clock == clock_values[index]) break;
  }
  if (step > 0) {
    index++;
    if (index >= MAX_CLOCK_VALUES) index = 0;
    psp_cpu_clock = clock_values[index];

  } else {
    index--;

    if (index < 0) index = MAX_CLOCK_VALUES - 1;
    psp_cpu_clock = clock_values[index];
  }
}


void
psp_settings_menu_height( int step )
{
  if (step > 0) {
    screen_h++;
    if (screen_h > EDITOR_SCREEN_HEIGHT) {
      screen_h = EDITOR_SCREEN_MIN_HEIGHT;
    }
  } else {
    screen_h--;
    if (screen_h < EDITOR_SCREEN_MIN_HEIGHT) {
      screen_h = EDITOR_SCREEN_HEIGHT;
    }
  }
}

void
psp_settings_menu_bg_color( int step )
{
  if (step > 0) {
    bg_color++;
    if (bg_color >= EDITOR_MAX_COLOR) bg_color = 0;
  } else {
    bg_color--;
    if (bg_color < 0) bg_color = EDITOR_MAX_COLOR-1;
  }
}


void
psp_settings_menu_tabsize( int step )
{
  if (step < 0) {
    tab_stop--;
    if (tab_stop < 1) tab_stop = 10;
  } else {
    tab_stop++;
    if (tab_stop > 10) tab_stop = 1;
  }
}

void
psp_settings_menu_syntax( int step )
{
  if (step < 0) {
    psp_syntax_go_next();
  } else {
    psp_syntax_go_previous();
  }
}

void
psp_settings_menu_filename_add_char(int a_char)
{
  int length = strlen(PSPWRITE.edit_filename);
  if (length < (MAX_PATH-2)) {
    PSPWRITE.edit_filename[length] = a_char;
    PSPWRITE.edit_filename[length+1] = 0;
  }
}

void
psp_settings_menu_filename_clear()
{
  PSPWRITE.edit_filename[0] = 0;
}

void
psp_settings_menu_filename_del()
{
  int length = strlen(PSPWRITE.edit_filename);
  if (length >= 1) {
    PSPWRITE.edit_filename[length - 1] = 0;
  }
}

int 
psp_settings_menu(void)
{
  SceCtrlData c;
  long        new_pad;
  long        old_pad;
  int         last_time;
  int         end_menu;

  psp_kbd_wait_no_button();
  psp_sdl_select_font( 1 );

  old_pad   = 0;
  last_time = 0;
  end_menu  = 0;

  psp_settings_menu_init();

  while (! end_menu)
  {
    psp_display_screen_settings_menu();
    psp_sdl_flip();

    while (1) {

      myCtrlPeekBufferPositive(&c, 1);
      c.Buttons &= PSP_ALL_BUTTON_MASK;

# ifdef USE_PSP_IRKEYB
      psp_irkeyb_set_psp_key(&c);
# endif
      if (c.Buttons) break;
    }

    new_pad = c.Buttons;

    if ((old_pad != new_pad) || ((c.TimeStamp - last_time) > PSP_MENU_MIN_TIME)) {
      last_time = c.TimeStamp;
      old_pad = new_pad;

    } else continue;

    if ((c.Buttons & (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) ==
        (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) {
      /* Exit ! */
      psp_sdl_exit(0);
    } else
    if ((c.Buttons & PSP_CTRL_RTRIGGER) == PSP_CTRL_RTRIGGER) {
      psp_settings_menu_reset();
      end_menu = 1;
    } else
    if ((new_pad & PSP_CTRL_CROSS ) || 
        (new_pad & PSP_CTRL_CIRCLE) || 
        (new_pad & PSP_CTRL_RIGHT ) ||
        (new_pad & PSP_CTRL_LEFT  )) 
    {
      int step = 0;

      if (new_pad & (PSP_CTRL_RIGHT|PSP_CTRL_CROSS)) {
        step = 1;
      } else
      if (new_pad & (PSP_CTRL_LEFT|PSP_CTRL_CIRCLE)) {
        step = -1;
      }

      switch (cur_menu_id ) 
      {
        case MENU_SET_TAB_SIZE : psp_settings_menu_tabsize( step );
        break;              
        case MENU_SET_SYNTAX   : psp_settings_menu_syntax( step );
        break;              
        case MENU_SET_FG_COLOR : psp_settings_menu_color( step );
        break;              
        case MENU_SET_BG_COLOR : psp_settings_menu_bg_color( step );
        break;              
        case MENU_SET_WORD_WRAP: wrap_mode = ! wrap_mode;
        break;              
        case MENU_SET_WRAP_WIDTH:  psp_settings_menu_wrap_width( step );
        break;              
        case MENU_SET_WIDTH    : psp_settings_menu_width( step );
        break;              
        case MENU_SET_FONT_SIZE: psp_settings_menu_font_size( step );
        break;              
        case MENU_SET_HEIGHT   : psp_settings_menu_height( step );
        break;              
        case MENU_SET_DOS_MODE : dos_mode = ! dos_mode;
        break;              
        case MENU_SET_MOVE_CURSOR : move_on_text = ! move_on_text;
        break;              
        case MENU_SET_EXPAND_TAB : expand_tab = ! expand_tab;
        break;              
        case MENU_SET_CLOCK    : psp_settings_menu_clock( step );
        break;              
        case MENU_SET_LOAD       : psp_settings_menu_load_config();
                                   old_pad = new_pad = 0;
        break;                     
        case MENU_SET_SAVE       : psp_settings_menu_save_config();
                                   old_pad = new_pad = 0;
        break;                     
        case MENU_SET_RESET      : psp_settings_menu_reset();
        break;                     
                                   
        case MENU_SET_BACK       : end_menu = 1;
        break;                     
      }

    } else
    if(new_pad & PSP_CTRL_UP) {

      if (cur_menu_id > 0) cur_menu_id--;
      else                 cur_menu_id = MAX_MENU_SET_ITEM-1;

    } else
    if(new_pad & PSP_CTRL_DOWN) {

      if (cur_menu_id < (MAX_MENU_SET_ITEM-1)) cur_menu_id++;
      else                                 cur_menu_id = 0;

    } else  
    if(new_pad & PSP_CTRL_SQUARE) {
      /* Cancel */
      end_menu = -1;
    } else 
    if(new_pad & PSP_CTRL_SELECT) {
      /* Back to Console */
      end_menu = 1;
    } else
    if(new_pad & PSP_CTRL_RTRIGGER) {
      /* TO_BE_DONE ! */
    } else
    if(new_pad & PSP_CTRL_LTRIGGER) {
      /* TO_BE_DONE ! */
    }
  }

  if (end_menu > 0) {
    psp_settings_menu_validate();
  }
 
  psp_editor_update_column();
  psp_editor_update_line();

  psp_kbd_wait_no_button();

  return 1;
}
