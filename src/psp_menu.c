/*
 *  Copyright (C) 2007 Ludovic Jacomme (ludovic.jacomme@gmail.com)
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
#include "psp_menu_set.h"
#include "psp_battery.h"
#include "psp_help.h"
#include "psp_irkeyb.h"
#include "psp_fmgr.h"
#include "psp_editor.h"

# define MENU_FILENAME     0
# define MENU_MODE         1
# define MENU_HELP         2
# define MENU_NEW          3
# define MENU_LOAD         4
# define MENU_RECENT       5
# define MENU_SAVE         6
# define MENU_SETTINGS     7

# define MENU_BACK         8
# define MENU_EXIT         9

# define MAX_MENU_ITEM (MENU_EXIT + 1)

  static menu_item_t menu_list[] =
  {
    { "File: " },
    { "Mode: " },

    { "Help" },

    { "New" },
    { "Load" },
    { "Recent" },
    { "Save" },

    { "Settings" },

    { "Back" },
    { "Exit" }
  };

  static int cur_menu_id = MENU_NEW;

void
string_fill_with_space(char *buffer, int size)
{
  int length = strlen(buffer);
  int index;

  for (index = length; index < size; index++) {
    buffer[index] = ' ';
  }
  buffer[size] = 0;
}

void
psp_display_screen_menu_battery(void)
{
  char buffer[64];

  int Length;
  int color;

  snprintf(buffer, 50, " [%s] ", psp_get_battery_string());
  Length = strlen(buffer);

  if (psp_is_low_battery()) color = PSP_MENU_RED_COLOR;
  else                      color = PSP_MENU_BORDER_COLOR;

  psp_sdl_back_print(240 - ((8*Length) / 2), 6, buffer, color);
}

# define FILENAME_FIELD_WIDTH    48

void 
psp_display_screen_menu(void)
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

    psp_sdl_back_print(  30, 6, " Menu ", PSP_MENU_NOTE_COLOR);

    psp_display_screen_menu_battery();

    psp_sdl_back_print(30, 254, " O/X: Valid  Sel: Back  Start: Keyboard ", 
                       PSP_MENU_BORDER_COLOR);

    psp_sdl_back_print(370, 254, " By Zx-81 ",
                       PSP_MENU_AUTHOR_COLOR);
  }
  
  x      = 20;
  y      = 25;
  y_step = 10;
  
  for (menu_id = 0; menu_id < MAX_MENU_ITEM; menu_id++) {
    color = PSP_MENU_TEXT_COLOR;
    if (cur_menu_id == menu_id) color = PSP_MENU_SEL_COLOR;
    else 
    if (menu_id == MENU_EXIT) color = PSP_MENU_WARNING_COLOR;
    else
    if (menu_id == MENU_HELP) color = PSP_MENU_GREEN_COLOR;

    psp_sdl_back_print(x, y, menu_list[menu_id].title, color);

    if (menu_id == MENU_FILENAME) {
      int length = strlen(PSPWRITE.edit_filename);
      int first = 0;
      if (length > (FILENAME_FIELD_WIDTH-1)) {
        first = length - (FILENAME_FIELD_WIDTH-1);
      }
      sprintf(buffer, "%s", PSPWRITE.edit_filename + first);
      if (menu_id == cur_menu_id) strcat(buffer, "_");
      string_fill_with_space(buffer, FILENAME_FIELD_WIDTH+1);
      psp_sdl_back_print( 70, y, buffer, color);

      y += y_step;
    } else
    if (menu_id == MENU_MODE) {
      if (PSPWRITE.is_read_only) strcpy(buffer, "read-only");
      else                       strcpy(buffer, "edit");
      string_fill_with_space(buffer, 12);
      psp_sdl_back_print( 70, y, buffer, color);

      y += y_step;
    } else
    if (menu_id == MENU_HELP) {
      y += y_step;
    } else
    if (menu_id == MENU_SAVE) {
      y += y_step;
    } else
    if (menu_id == MENU_BACK) {
      y += y_step;
    }

    y += y_step;
  }
}

int
psp_edit_menu_confirm_overwrite()
{
  SceCtrlData c;

  if (PSPWRITE.ask_overwrite) {

    struct stat aStat;
    if (stat(PSPWRITE.edit_filename, &aStat)) {
      PSPWRITE.ask_overwrite = 0;
      return 1;
    }

    psp_display_screen_menu();
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

static int
psp_edit_menu_save()
{
  int error;

  if (PSPWRITE.ask_overwrite) {
    if (! psp_edit_menu_confirm_overwrite()) {
      return 0;
    }
  }
  PSPWRITE.ask_overwrite = 0;
  error = psp_editor_save();

  if (! error) /* save OK */
  {
    psp_display_screen_menu();
    psp_sdl_back_print(140, 160, "File saved !", 
                       PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  else 
  {
    psp_display_screen_menu();
    psp_sdl_back_print(140, 160, "Can't save file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  return error;
}

int
psp_edit_menu_confirm_save()
{
  SceCtrlData c;

  if (PSPWRITE.is_modified) {

    psp_display_screen_menu();
    psp_sdl_back_print(220, 70, "Save last change ? X to save", PSP_MENU_WARNING_COLOR);
    psp_sdl_back_print(220, 80, "O to ignore, [] to cancel", PSP_MENU_WARNING_COLOR);
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
        if (psp_edit_menu_save()) {
          /* failed -> cancel */
          return 0;
        }
      } else 
      if (c.Buttons & PSP_CTRL_SQUARE) {
        return 0;
      } else 
      if (c.Buttons & PSP_CTRL_CIRCLE) {
        break;
      }

    } while (c.Buttons == 0);
    psp_kbd_wait_no_button();
  }
  return 1;
}

int
psp_edit_menu_exit(void)
{
  if (! psp_edit_menu_confirm_save()) {
    return 0;
  }

  SceCtrlData c;

  psp_display_screen_menu();
  psp_sdl_back_print(270, 70, "Press X to exit !", PSP_MENU_WARNING_COLOR);
  psp_sdl_flip();

  psp_kbd_wait_no_button();

  do
  {
    myCtrlPeekBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;

    if (c.Buttons & PSP_CTRL_CROSS) {
      psp_global_save_config();
      psp_sdl_exit(0);
    }

  } while (c.Buttons == 0);

  psp_kbd_wait_no_button();

  return 0;
}

int
psp_edit_menu_new(void)
{
  if (psp_edit_menu_confirm_save()) {
    psp_editor_new();
  }
  return 0;
}

int
psp_edit_menu_load(void)
{
  int ret;

  if (! psp_edit_menu_confirm_save()) {
    return 0;
  }
  ret = psp_fmgr_menu(PSP_FMGR_FORMAT_TXT);
  if (ret ==  1) /* load OK */
  {
    psp_display_screen_menu();
    psp_sdl_back_print(270,  60, "File loaded !", PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
    return 1;
  }
  else 
  if (ret == -1) /* Load Error */
  {
    psp_display_screen_menu();
    psp_sdl_back_print(270,  60, "Can't load file !", PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  return 0;
}

void
psp_edit_menu_filename_add_char(int a_char)
{
  int length = strlen(PSPWRITE.edit_filename);
  if (length < (MAX_PATH-2)) {
    PSPWRITE.edit_filename[length] = a_char;
    PSPWRITE.edit_filename[length+1] = 0;
  }
}

void
psp_edit_menu_filename_clear()
{
  PSPWRITE.edit_filename[0] = 0;
}

void
psp_edit_menu_filename_del()
{
  int length = strlen(PSPWRITE.edit_filename);
  if (length >= 1) {
    PSPWRITE.edit_filename[length - 1] = 0;
  }
}

int 
psp_edit_main_menu(void)
{
  SceCtrlData c;
  long        new_pad;
  long        old_pad;

  int         last_time;
  int         end_menu;

  int         danzeff_mode = 0;
  int         danzeff_key;

  int         step = 0;

  psp_kbd_wait_no_button();
  psp_sdl_select_font( 1 );

  old_pad   = 0;
  last_time = 0;
  end_menu  = 0;

# ifdef USE_PSP_IRKEYB
  int irkeyb_key = PSP_IRKEYB_EMPTY;
# endif

  while (! end_menu)
  {
    psp_display_screen_menu();

    if (danzeff_mode) {
      danzeff_moveTo(-50, -50);
      danzeff_render();
    }
    psp_sdl_flip();

    while (1) {

      myCtrlPeekBufferPositive(&c, 1);
      c.Buttons &= PSP_ALL_BUTTON_MASK;

# ifdef USE_PSP_IRKEYB
      irkeyb_key = psp_irkeyb_set_psp_key(&c);
# endif
      new_pad = c.Buttons;

      if ((old_pad != new_pad) || ((c.TimeStamp - last_time) > PSP_MENU_MIN_TIME)) {
        last_time = c.TimeStamp;
        old_pad = new_pad;
        break;
      }
# ifdef USE_PSP_IRKEYB
      if (irkeyb_key != PSP_IRKEYB_EMPTY) break;
# endif
    }

# ifdef USE_PSP_IRKEYB
    irkeyb_key = psp_irkeyb_read_key();

    if (irkeyb_key > 0) {
      if (irkeyb_key > ' ') {
        psp_edit_menu_filename_add_char(irkeyb_key);
      } else
      if (irkeyb_key == 0xc) {
        psp_edit_menu_filename_clear(irkeyb_key);
      } else
      if (irkeyb_key == 0x8) {
        psp_edit_menu_filename_del(irkeyb_key);
      } else
      if (irkeyb_key == 0xd) {
        new_pad |= PSP_CTRL_CROSS;
      } else
      if (irkeyb_key == 0x1d) {
        new_pad |= PSP_CTRL_SELECT;
      }
      irkeyb_key = PSP_IRKEYB_EMPTY;
    }
# endif
    if (danzeff_mode) {

      danzeff_key = danzeff_readInput(c);
      if (danzeff_key > DANZEFF_START) {
        /* Disable utf8 */
        danzeff_key &= 0x7f;
        if (danzeff_key > ' ') {
          psp_edit_menu_filename_add_char(danzeff_key);
        } else
        if (danzeff_key == DANZEFF_CLEAR) {
          psp_edit_menu_filename_clear();
        } else
        if (danzeff_key == DANZEFF_DEL) {
          psp_edit_menu_filename_del();
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
    if ((new_pad & (PSP_CTRL_CROSS|PSP_CTRL_RIGHT)) || 
        (new_pad & (PSP_CTRL_CIRCLE|PSP_CTRL_LEFT)))
    {
      if (new_pad & (PSP_CTRL_RIGHT|PSP_CTRL_CROSS)) {
        step = 1;
      } else
      if (new_pad & (PSP_CTRL_LEFT|PSP_CTRL_CIRCLE)) {
        step = -1;
      }

      switch (cur_menu_id ) 
      {
        case MENU_LOAD      : if (psp_edit_menu_load()) {
                                end_menu = 1;
                              }
        break;              
        case MENU_RECENT    : if (psp_recent_menu()) {
                                end_menu = 1;
                              }
        break;              
        case MENU_NEW       : psp_edit_menu_new();
                              end_menu = 1;
        break;              
        case MENU_MODE      : PSPWRITE.is_read_only = ! PSPWRITE.is_read_only;
        break;              

        case MENU_SAVE      : psp_edit_menu_save();
        break;              

        case MENU_SETTINGS   : psp_settings_menu();
                               old_pad = new_pad = 0;
        break;

        case MENU_BACK      : end_menu = 1;
        break;

        case MENU_EXIT      : psp_edit_menu_exit();
        break;

        case MENU_HELP      : psp_help_menu();
                              old_pad = new_pad = 0;
        break;              

        case MENU_FILENAME : if (step < 0) psp_edit_menu_filename_del();
        break;
      }

    } else
    if(new_pad & PSP_CTRL_START) {
      danzeff_mode = 1;
    } else
    if(new_pad & PSP_CTRL_UP) {

      if (cur_menu_id > 0) cur_menu_id--;
      else                 cur_menu_id = MAX_MENU_ITEM-1;

    } else
    if(new_pad & PSP_CTRL_DOWN) {

      if (cur_menu_id < (MAX_MENU_ITEM-1)) cur_menu_id++;
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

  psp_kbd_wait_no_button();

  return 0;
}

