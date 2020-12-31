
    Welcome to PSPWrite

Written by Ludovic.Jacomme also known as Zx-81 (zx81.zx81@gmail.com)

1. INTRODUCTION
   ------------

  PSPWrite is a simple ASCII text editor, nothing compared to Open office, 
  but good enough to edit any text file (even big files) on your PSP.

  This package is under GPL Copyright, read COPYING file for
  more information about it.


2. INSTALLATION
   ------------

  Unzip the zip file, and copy the content of the directory fw5x or fw15
  (depending of the version of your firmware) on the psp/game150 or
  psp/game5XX directory.

  It has been developped on linux for Firmwares 3, 4 or 5.x-M33

  Thanks to Danzel for his Virtual kerboard

  For any comments or questions on this version, please visit 
  http://zx81.zx81.free.fr, http://www.dcemu.co.uk, or 
  http://pspupdates.qj.net


3. CONTROL
   ------------

3.1 - Virtual keyboard

  In the editor window, press Start to open/close the 
  On-Screen keyboard

  The On-Screen Keyboard of "Danzel" and "Jeff Chen"

  Use Analog stick to choose one of the 9 squares, and
  use Triangle, Square, Cross and Circle to choose one
  of the 4 letters of the highlighted square.

  While the virtual keyboard is displayed, you can still
  use the digital pad :

  Left    Move cursor left
  Right   Move cursor right
  Up      Move cursor to the beginning of the line
  Down    Return
  Select  Disable virtual keyboard
  Start   Disable virtual keyboard

3.2 - Standard keys

  When the virtual keyboard is off then the following
  mapping is done :

  Up          Move cursor up
  Down        Move cursor down
  Left        Move cursor left
  Right       Move cursor right
  Triangle    Backspace
  Square      Delete
  Circle      Space 
  Cross       Return
  Select      Menu
  Start       Virtual keyboard
  
  L+Up        First line
  L+Down      Last line
  L+Right     End of the line
  L+Left      Beginning of the line

  L+Select    Toggle Command/Edit

  L+Triangle  First line
  L+Cross     Last line
  L+Square    Beginning of the line
  L+Circle    End of the line
  
  R+Up        Page up
  R+Down      Page down
  R+Left      Word left
  R+Right     Word right

  R+Select    Selection mode

  R+Triangle  Copy
  R+Cross     Cut
  R+Square    Rewrap paragraph
  R+Circle    Paste

  In command mode :

  Left        Choose next command or validate 
  Right       Choose previous command or validate
  Up          Go up in command history
  Down        Go down in command history


3.3 - IR keyboard

  You can also use IR keyboard. Edit the pspirkeyb.ini file
  to specify your IR keyboard model, and modify eventually
  layout keyboard files in the keymap directory.

  The following mapping is done :

  IR-keyboard   PSP

  Cursor        Digital Pad

  Tab           Tab   
  Ctrl-W        Start

  Escape        Toggle Command/Edit
  Ctrl-Q        Select

  Ctrl-E        Triangle
  Ctrl-X        Cross
  Ctrl-S        Square
  Ctrl-F        Circle

  Ctrl-L        Clear line
  Ctrl-C        Copy
  Ctrl-V        Selection mode
  Ctrl-D        Cut/Delete
  Ctrl-P        Paste
  Ctrl-B        Word left
  Ctrl-N        Word Right

When the command mode is activated (L+Select) then a ':' is prompted in bottom
left corner. 
You can then use the following "VI like" commands :

- '/pattern' to search forward for a pattern in the file from current cursor
  position.  If you press X or <ENTER> then you will go to next occurence.
  IF you want to escape the command mode then press <ESCAPE> or L+Select.

- '?pattern' to search backward for a pattern in the file from current cursor
  position.  If you press X or <ENTER> then you will go to next occurence.
  IF you want to escape the command mode then press <ESCAPE> or L+Select.

- 'number_line' to go directly to 'number_line' position.

In command mode you can use the left/rigth pad in first position to select 
the command you want and then use 'space' to select that command.
You can also use up/down pad to go in command history.


4. SETTINGS
   ------------

  The editor menu let you change colors and some other options.
  If you want to change the background or foreground color in 
  the editor window or to toggle between DOS mode (with \r\n 
  characters for cariage return), to expand tabulations etc ...
 
  WARNING: This editor will replace all tabulation by spaces when
  the expand tab option is set. You can specify the number of 
  spaces in options menu.

  You can use the virtual keyboard in the file requester menu 
  to choose the first letter of the file you search.

5. SYNTAX
   ------------

Depending of the file extention, PSPWrite will use the syntax.cfg
configuration file to automatically colorised keywords etc ... 
For now it supports only Lua and C/C++ syntax, but you can add your 
own in syntax.cfg file, taken C/C++ as an example. 


6. RECENT FILES 
   ------------

All previously loaded files should appear in the Recent files menu. 
It might help to go directly to the position you where when 
you last edit that file.
  

7. COMPILATION
   ------------

It has been developped under Linux using gcc with PSPSDK. 
To rebuild the homebrew run the Makefile in the src archive.


  Enjoy,
  
         Zx
