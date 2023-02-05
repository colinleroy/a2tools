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
#include "scrollwindow.h"

#ifdef __CC65__
#pragma code-name (push, "LC")
#endif

account *my_account = NULL;

void print_header(list *l, status *root_status, notification *root_notif) {
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

  #define BTM 3
  gotoxy(0,BTM);

  dputs("Commands:          \r\n"
        " View toot: Enter  \r\n"
        " Scroll   : Up/dn  \r\n"
        " Search   : S      \r\n"
        " Notifs.  : N      \r\n"
        " Timelines: H/L/G  \r\n"
        " Configure: O      \r\n"
        " Back     : Escape \r\n");

  if (root_status) {
    dputs("Toot:              \r\n"
          " Reply    : R      \r\n"
          " Images   : I      \r\n");
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
    if (my_account && !strcmp(root_status->account->id, my_account->id)) {
      dputs(" Delete   : D      \r\n");
    }
    dputs("Author:            \r\n"
          " Profile  : P      \r\n");
  } else if (l && l->account) {
    dputs("Profile:           \r\n"
          " Images   : I      \r\n");
  } else if (root_notif) {
    dputs("Profile:            \r\n"
          " Open     : P       \r\n");
  }
  dputs("Writing:           \r\n"
        " Compose  : C      \r\n");

  while (wherey() < 21) {
    dputs("                   \r\n");
  }

  print_free_ram();
  cvlinexy(LEFT_COL_WIDTH, 0, scrh);
}

void print_free_ram(void) {
#ifdef __CC65__
  gotoxy(0, 22);
  cprintf("%zuB free     \r\n"
          "%zuB max      ",
          _heapmemavail(), _heapmaxavail());
#endif
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
