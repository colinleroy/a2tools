#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "surl.h"
#include "extended_conio.h"
#include "strsplit.h"
#include "scroll.h"
#include "cli.h"
#include "api.h"
#include "header.h"
#include "list.h"
#include "math.h"
#include "dgets.h"
#include "dputs.h"
#include "dputc.h"
#include "clrzone.h"
#include "scrollwindow.h"

account *my_account = NULL;

void compose_print_header(void) {
  if (my_account == NULL) {
    my_account = api_get_profile(NULL);
  }
  if (my_account != NULL) {
    if (strlen(my_account->display_name) > LEFT_COL_WIDTH)
      my_account->display_name[LEFT_COL_WIDTH] = '\0';

    if (strlen(my_account->username) > LEFT_COL_WIDTH)
      my_account->username[LEFT_COL_WIDTH] = '\0';

    gotoxy(0, 0);
    dputs(my_account->display_name);
    gotoxy(0, 1);
    dputc(arobase);
    dputs(my_account->username);
  }

  #define BTM 4
  clrzone(0, BTM, LEFT_COL_WIDTH, 23);

  dputs("Commands:\r\n"
        " Open-Apple +...\r\n"
        " Send     : S\r\n"
        " Images   : I\r\n"
        " CW       : C\r\n"
        " Cancel   : Escape\r\n"
        "\r\n"
        "Set Audience:\r\n"
        " Open-Apple +...\r\n"
        " Public   : P\r\n"
        " Unlisted : U\r\n"
        " Private  : R\r\n"
        " Mention  : M\r\n"
        "\r\n"
      );

  print_free_ram();
  cvlinexy(LEFT_COL_WIDTH, 0, scrh);
}

void print_free_ram(void) {
#ifdef __CC65__
  unsigned char sx, wx;

  get_hscrollwindow(&sx, &wx);
  set_hscrollwindow(0, scrw);

  gotoxy(0, 23);
  cprintf("%zuB free     ",
          _heapmemavail());

  set_hscrollwindow(sx, wx);
#endif
}
