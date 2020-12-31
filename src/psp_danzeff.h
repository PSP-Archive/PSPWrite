#ifndef INCLUDED_KEYBOARDS_DANZEFF_H
#define INCLUDED_KEYBOARDS_DANZEFF_H

//danzeff is BSD licensed, if you do make a new renderer then please share it back and I can add it
//to the original distribution.

//Set which renderer target to build for (currently only SDL is available)
#define DANZEFF_SDL
//TODO #define DANZEFF_SCEGU etc etc ;)

#define DANZEFF_SELECT          1
#define DANZEFF_START           2

#define DANZEFF_LEFT            4
#define DANZEFF_RIGHT           5
#define DANZEFF_UP              6
#define DANZEFF_DOWN            7
#define DANZEFF_DEL             8
#define DANZEFF_TAB             9
#define DANZEFF_CLEAR          12
#define DANZEFF_ENTER          13

#define DANZEFF_ESCAPE         14
#define DANZEFF_PAGE_UP        15
#define DANZEFF_PAGE_DOWN      16
#define DANZEFF_END            17
#define DANZEFF_HOME           18
#define DANZEFF_INSERT         19

#define DANZEFF_F1             21
#define DANZEFF_F2             22
#define DANZEFF_F3             23
#define DANZEFF_F4             24
#define DANZEFF_F5             25
#define DANZEFF_F6             26
#define DANZEFF_F7             27
#define DANZEFF_F8             28
#define DANZEFF_F9             29

//the SDL implementation needs the pspctrl_emu wrapper to convert
//a SDL_Joystick into a SceCtrlData
#include <pspctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

 extern int   psp_kbd_skin;
 extern int   psp_kbd_last_skin;
 extern char *psp_kbd_skin_dir[];

//Initialization and de-init of the keyboard, provided as the keyboard uses alot of images, so if you aren't going to use it for a while, I'd recommend unloading it.
extern int danzeff_load(void);
extern void danzeff_free(void);

//returns true if the keyboard is initialized
extern int danzeff_isinitialized(void);

/** Attempts to read a character from the controller
* If no character is pressed then we return 0
* Other special values: 1 = move left, 2 = move right, 3 = select, 4 = start
* Every other value should be a standard ascii value.
* An unsigned int is returned so in the future we can support unicode input
*/
extern int danzeff_readInput(SceCtrlData pspctrl);

//Move the area the keyboard is rendered at to here
extern void danzeff_moveTo(const int newX, const int newY);

//Returns true if the keyboard that would be rendered now is different to the last time
//that the keyboard was drawn, this is altered by readInput/render.
extern   int danzeff_dirty();

//draw the keyboard to the screen
extern void danzeff_render();

///Functions only for particular renderers:

#include <SDL/SDL.h>
//set the screen surface for rendering on.
extern void danzeff_set_screen(SDL_Surface* screen);


#ifdef __cplusplus
}
#endif

#endif //INCLUDED_KEYBOARDS_DANZEFF_H
