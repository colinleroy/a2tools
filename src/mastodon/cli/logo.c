#include <string.h>
#include "scrollwindow.h"
#include "logo.h"
#include "extended_conio.h"

void print_logo(unsigned char scrw) {
#ifdef __APPLE2ENH__
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
#else
  char *logo =
    "  **********\r\n"
    " **        **        WELCOME TO\r\n"
    "**  **  **  **        MASTODON\r\n"
    "**  **  **  **\r\n"
    "**  ******  **   (c) Colin Leroy-Mira\r\n"
    " ************       https://colino.net\r\n"
    "  *********\r\n"
    "   **           This program is GPL v3.\r\n"
    "     ****'\r\n";

  /* 58 is the width of char *logo */
  gotoxy(0, 2);
  cputs(logo);
#endif
  set_hscrollwindow(0, scrw);
  cputs("\r\n");
  chline(scrw);
  cputs("\r\n");
}
