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
#include "dputs.h"
#include "dputc.h"
#include "clrzone.h"
#include "scrollwindow.h"

account *my_account = NULL;
char *key_combo = "Ctrl";

void compose_show_help(void) {
  #define BTM 1
  clrzone(0, BTM, LEFT_COL_WIDTH, 23);

  if (is_iie) {
    key_combo = "Open-Apple";
  }

  dputs("Commands:\r\n");
  dputs(key_combo);
  dputs(" +...\r\n"
        " Send      : S\r\n"
        " Images    : I\r\n"
        " CW        : C\r\n"
        " Poll      : V\r\n"
        " Cancel    : X\r\n"
        "\r\n"
        "Set Audience:\r\n");
  dputs(key_combo);
  dputs(" +...\r\n"
        " Public    : P\r\n"
        " Unlisted  : L\r\n"
        " Private   : R\r\n"
        " Direct    : D\r\n"
        "\r\n"
        "Quote Policy:\r\n");
  dputs(key_combo);
  dputs(" +...\r\n"
        " Public    : E\r\n"
        " Followers : F\r\n"
        " Nobody    : N\r\n");

  print_free_ram();
}

void compose_print_header(void) {
  if (IS_NULL(my_account)) {
    my_account = api_get_profile(NULL);
  }

  if (has_80cols) {
    if (IS_NOT_NULL(my_account)) {
      if (strlen(my_account->username) > LEFT_COL_WIDTH)
        my_account->username[LEFT_COL_WIDTH] = '\0';

      gotoxy(0, 0);
      dputc(arobase);
      dputs(my_account->username);
    }

    compose_show_help();
    cvlinexy(LEFT_COL_WIDTH, 0, NUMLINES);
  }
}

void print_free_ram(void) {
  unsigned char sx, wx;

  if (has_80cols) {
    get_hscrollwindow(&sx, &wx);
    set_hscrollwindow(0, NUMCOLS);
  }
#ifdef __CC65__
  gotoxy(0, 23);
  cprintf("%zuB free     ",
          _heapmemavail());
#endif

  if (has_80cols) {
    set_hscrollwindow(sx, wx);
  }
}
