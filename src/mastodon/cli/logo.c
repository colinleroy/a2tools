#include <string.h>
#include "scrollwindow.h"
#include "logo.h"
#include "extended_conio.h"

void print_logo(unsigned char scrw) {
  char *logo = 
    "   ***************\r\n"
    " ***      '      ***       WELCOME TO MASTODON\r\n"
    "***   ***   ***   ***\r\n"
    "***   ***   ***   ***      (c) Colin Leroy-Mira, 2023\r\n"
    "***   *********   ***          <https://www.colino.net>\r\n"
    " *******************\r\n"
    "  ****************         This program is free software,\r\n"
    "   ****                    distributed under the GPL v3.\r\n"
    "     *********'\r\n";

  /* 58 is the width of char *logo */
  set_hscrollwindow((scrw - 58) / 2, 58);
  gotoxy(0, 2);
  cputs(logo);
  set_hscrollwindow(0, scrw);
  cputs("\r\n");
  chline(scrw);
  cputs("\r\n");
}
