#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#ifdef __APPLE2__
#include <apple2enh.h>
#endif
#include "surl.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "dputs.h"
#include "dputc.h"
#include "scroll.h"
#include "cli.h"
#include "api.h"
#include "header.h"
#include "list.h"
#include "math.h"
#include "dgets.h"


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

    cputsxy(0, 0, my_account->display_name);
    gotoxy(0, 1);
    cprintf("%c%s\r\n", arobase, my_account->username);
  }

  #define BTM 10
  clrzone(0, BTM, LEFT_COL_WIDTH, 23);
  gotoxy(0,BTM);

  cputs("General:\r\n");
  cputs(" View toot: Enter\r\n");
  cputs(" Scroll   : Up/dn\r\n");
  if (!l || !l->root) {
    cputs(" Configure: O\r\n");
    cputs(" Exit     : Escape \r\n");
  } else {
    cputs(" Back     : Escape \r\n");
    
    if (root_status) {
      cputs("Toot: \r\n");
      if ((root_status->favorited_or_reblogged & FAVOURITED) != 0) {
        cputs(" Unfav.   : F      \r\n");
      } else {
        cputs(" Favourite: F      \r\n");
      }
      if ((root_status->favorited_or_reblogged & REBLOGGED) != 0) {
        cputs(" Unboost  : B      \r\n");
      } else {
        cputs(" Boost    : B      \r\n");
      }
      if (!strcmp(root_status->account->id, my_account->id)) {
        cputs(" Delete   : D      \r\n");
      }

      cputs("Author:\r\n");
      cputs(" Profile  : P      \r\n");
    }
  }
  cputs("Writing:\r\n");
  cputs(" Compose  : C      \r\n");

  print_free_ram();
  cvlinexy(LEFT_COL_WIDTH, 0, scrh);
}

void print_free_ram(void) {
#ifdef __CC65__
  gotoxy(0, 23);
  cprintf("%zuB free     ", _heapmemavail());
#endif
}
