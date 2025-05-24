#include <string.h>
#include "scrollwindow.h"
#include "cli.h"
#include "logo.h"
#include "extended_conio.h"

void print_logo(void) {
  static char *logo;
  if (has_80cols) {
    logo =
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
    set_hscrollwindow((NUMCOLS - 58) / 2, 58);
  } else {
    logo =
      "  **********\r\n"
      " **        **        WELCOME TO\r\n"
      "**  **  **  **        MASTODON\r\n"
      "**  **  **  **\r\n"
      "**  ******  **   (c) Colin Leroy-Mira\r\n"
      " ************       https://colino.net\r\n"
      "  *********\r\n"
      "   **           This program is GPL v3.\r\n"
      "     ****'\r\n";
    }
  gotoxy(0, 2);
  cputs(logo);

  set_hscrollwindow(0, NUMCOLS);
  cputs("\r\n");
  chline(NUMCOLS);
  cputs("\r\n");
}
