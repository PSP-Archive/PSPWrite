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

# ifndef _PSP_MENU_H_
# define _PSP_MENU_H_


# define PSP_MENU_BORDER_COLOR     psp_sdl_rgb(0x10,0x10,0x10)
# define PSP_MENU_WARNING_COLOR    psp_sdl_rgb(0x70,0x20,0x0a)
# define PSP_MENU_NOTE_COLOR       psp_sdl_rgb(0xa0,0xa0,0x00)
# define PSP_MENU_BACKGROUND_COLOR psp_sdl_rgb(0x00,0x00,0x00)
# define PSP_MENU_BLACK_COLOR      psp_sdl_rgb(0x00,0x00,0x00)
# define PSP_MENU_WHITE_COLOR      psp_sdl_rgb(0xff,0xff,0xff)
# define PSP_MENU_AUTHOR_COLOR     psp_sdl_rgb(0x0a,0x20,0x70)
# define PSP_MENU_GREEN_COLOR      psp_sdl_rgb(0x0a,0x70,0x20)
# define PSP_MENU_RED_COLOR        psp_sdl_rgb(0xa0,0x20,0x70)

# define PSP_MENU_TEXT_COLOR       psp_sdl_rgb(0x80,0x80,0x80)
//# define PSP_MENU_TEXT_COLOR       psp_sdl_rgb(0x50,0xa0,0xa0)
//# define PSP_MENU_TEXT_COLOR       psp_sdl_rgb(0xe7,0xcd,0xb9)

# define PSP_MENU_TEXT2_COLOR      psp_sdl_rgb(0x20,0x20,0x80)
//# define PSP_MENU_SEL_COLOR        psp_sdl_rgb(0xff,0xff,0xff)
//# define PSP_MENU_SEL_COLOR        psp_sdl_rgb(0x8e,0x5e,0x37)
//# define PSP_MENU_SEL_COLOR        psp_sdl_rgb(0x36,0x36,0xa0)
//# define PSP_MENU_SEL_COLOR        psp_sdl_rgb(0x30,0xc0,0xc0)
#define PSP_MENU_SEL_COLOR         psp_sdl_rgb(0x30,0x30,0xa0)

# define PSP_MENU_SEL2_COLOR       psp_sdl_rgb(0xFF,0x00,0x80)
# define PSP_MENU_HELP_COLOR       psp_sdl_rgb(0x50,0x10,0x50)

# define PSP_MENU_MIN_TIME         150000

  typedef struct menu_item_t {
    char *title;
  } menu_item_t;


  extern int psp_edit_main_menu(void);
  extern void psp_display_screen_menu_battery(void);
  extern void string_fill_with_space(char *buffer, int size);
  extern void psp_edit_menu_banlist(char *chan_name);

# endif
