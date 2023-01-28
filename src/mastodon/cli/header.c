#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#ifdef __APPLE2__
#include <apple2enh.h>
#endif
#include "surl.h"
#ifdef __CC65__
#include <conio.h>
#else
#include "extended_conio.h"
#endif
#include "strsplit.h"
#include "dputs.h"
#include "dputc.h"
#include "scroll.h"
#include "cli.h"
#include "api.h"
#include "header.h"
#include "list.h"
#include "math.h"
#include "dgets.h"
#include "clrzone.h"
#include "scrollwindow.h"

account *my_account = NULL;

void print_header(list *l, status *root_status) {
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

  #define BTM 9
  clrzone(0, BTM, LEFT_COL_WIDTH, 23);
  gotoxy(0,BTM);

  dputs("General:\r\n");
  dputs(" View toot: Enter\r\n");
  dputs(" Scroll   : Up/dn\r\n");
  if (!l || !l->root) {
    dputs(" Configure: O\r\n");
    dputs(" Exit     : Escape \r\n");
  } else {
    dputs(" Back     : Escape \r\n");
    
    if (root_status) {
      dputs("Toot: \r\n");
      dputs(" Reply    : R      \r\n");
      if ((root_status->favorited_or_reblogged & FAVOURITED) != 0) {
        dputs(" Unfav.   : F      \r\n");
      } else {
        dputs(" Favourite: F      \r\n");
      }
      if ((root_status->favorited_or_reblogged & REBLOGGED) != 0) {
        dputs(" Unboost  : B      \r\n");
      } else {
        dputs(" Boost    : B      \r\n");
      }
      if (!strcmp(root_status->account->id, my_account->id)) {
        dputs(" Delete   : D      \r\n");
      }

      dputs("Author:\r\n");
      dputs(" Profile  : P      \r\n");
    }
  }
  dputs("Writing:\r\n");
  dputs(" Compose  : C      \r\n");

  print_free_ram();
  cvlinexy(LEFT_COL_WIDTH, 0, scrh);
}

void print_free_ram(void) {
#ifdef __CC65__
  gotoxy(0, 23);
  cprintf("%zuB free     ", _heapmemavail());
#endif
}