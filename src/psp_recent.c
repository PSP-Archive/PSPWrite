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
#include "psp_menu_set.h"
#include "psp_battery.h"
#include "psp_help.h"
#include "psp_irkeyb.h"
#include "psp_editor.h"
#include "psp_recent.h"

# define FILENAME_FIELD_WIDTH  54

 static int cur_recent_id = 0;
 static int max_recent_id = 0;

static void 
psp_display_recent_menu(void)
{
  recent_file_t* recent_file;

  char buffer[512];

  int recent_id = 0;
  int color     = 0;
  int x         = 0;
  int y         = 0;
  int y_step    = 0;

  {
    psp_sdl_blit_menu();

    psp_sdl_draw_rectangle(10,10,459,249,PSP_MENU_BORDER_COLOR,0);
    psp_sdl_draw_rectangle(11,11,457,247,PSP_MENU_BORDER_COLOR,0);

    psp_sdl_back_print(  30, 6, " Recent files ", PSP_MENU_NOTE_COLOR);

    psp_display_screen_menu_battery();

    psp_sdl_back_print(30, 254, " O/X: Valid  Sel: Back ", 
                       PSP_MENU_BORDER_COLOR);

    psp_sdl_back_print(370, 254, " By Zx-81 ",
                       PSP_MENU_AUTHOR_COLOR);
  }
  
  x      = 20;
  y      = 25;
  y_step = 10;
  
  for (recent_id = 0; recent_id < MAX_RECENT_FILE; recent_id++) {
    recent_file = &PSPWRITE.recent_file[ recent_id ];
    if (! recent_file->filename[0]) break;

    color = PSP_MENU_TEXT_COLOR;
    if (cur_recent_id == recent_id) color = PSP_MENU_SEL_COLOR;

    int length = strlen(recent_file->filename);
    int first = 0;
    if (length > (FILENAME_FIELD_WIDTH-1)) {
      first = length - (FILENAME_FIELD_WIDTH-1);
    }
    sprintf(buffer, "%s", recent_file->filename + first);
    string_fill_with_space(buffer, FILENAME_FIELD_WIDTH+1);
    psp_sdl_back_print( 20, y, buffer, color);

    y += y_step;
  }
  max_recent_id = recent_id;
}

int
psp_recent_menu_load(void)
{
  char TmpFileName[MAX_PATH];
  int  error;

  strcpy(TmpFileName, PSPWRITE.recent_file[ cur_recent_id ].filename);

  if (! psp_edit_menu_confirm_save()) {
    return 0;
  }
  error = psp_editor_load( TmpFileName );
  if (! error) /* load OK */
  {
    psp_display_screen_menu();
    psp_sdl_back_print(270,  60, "File loaded !", PSP_MENU_NOTE_COLOR);
    psp_sdl_flip();
    sleep(1);
    return 1;
  }
  else 
  {
    psp_display_screen_menu();
    psp_sdl_back_print(270,  60, "Can't load file !", PSP_MENU_WARNING_COLOR);
    psp_sdl_flip();
    sleep(1);
  }
  return 0;
}

int 
psp_recent_menu(void)
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

  cur_recent_id = 0;

# if 0 //LUDO: DEBUG
  psp_editor_display_recent();
# endif

# ifdef USE_PSP_IRKEYB
  int irkeyb_key = PSP_IRKEYB_EMPTY;
# endif

  while (! end_menu)
  {
    psp_display_recent_menu();
    if (! max_recent_id) return 0;

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

    if ((c.Buttons & (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) ==
        (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) {
      /* Exit ! */
      psp_sdl_exit(0);
    } else
    if(new_pad & PSP_CTRL_UP) {

      if (cur_recent_id > 0) cur_recent_id--;
      else                   cur_recent_id = max_recent_id-1;

    } else
    if(new_pad & PSP_CTRL_DOWN) {

      if (cur_recent_id < (max_recent_id-1)) cur_recent_id++;
      else                                 cur_recent_id = 0;

    } else  
    if(new_pad & PSP_CTRL_SQUARE) {
      /* Cancel */
      end_menu = -1;
    } else 
    if(new_pad & PSP_CTRL_CROSS) {
      /* Load */
      end_menu = -1;
      if (psp_recent_menu_load()) end_menu = 1;

    } else 
    if(new_pad & PSP_CTRL_SELECT) {
      /* Back to Main menu */
      end_menu = -1;
    }
  }

  psp_kbd_wait_no_button();

  return (end_menu == 1);
}

