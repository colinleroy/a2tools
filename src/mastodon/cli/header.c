#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "surl.h"
#include "extended_conio.h"
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

#ifdef SURL_TO_LANGCARD
#pragma code-name (push, "LC")
#endif

account *my_account = NULL;

char __fastcall__ print_header(list *l, status *root_status, notification *root_notif) {
  if (my_account == NULL) {
    my_account = api_get_profile(NULL);
  }
  if (my_account == NULL) {
    return -1;
  }

#if NUMCOLS == 80
  if (strlen(my_account->display_name) > LEFT_COL_WIDTH)
    my_account->display_name[LEFT_COL_WIDTH] = '\0';

  if (strlen(my_account->username) > LEFT_COL_WIDTH)
    my_account->username[LEFT_COL_WIDTH] = '\0';

  show_help(l, root_status, root_notif);
  cvlinexy(LEFT_COL_WIDTH, 0, scrh);
#endif

  return 0;
}

#ifdef SURL_TO_LANGCARD
#pragma code-name (pop)
#endif

void show_help (list *l, status *root_status, notification *root_notif) {

  gotoxy(0, 0);
  dputs(my_account->display_name);
  gotoxy(0, 1);
  dputc(arobase);
  dputs(my_account->username);

  #define BTM 2
  gotoxy(0,BTM);

  dputs("Commands:          \r\n"
        " View toot : Enter \r\n"
#ifdef __APPLE2ENH__
        " Scroll    : Up/dn \r\n"
#else
        " Scroll    : U/J   \r\n"
#endif
        " Search    : S     \r\n"
        " Notifs.   : N     \r\n"
        " Timelines : H/L/G \r\n"
        " Bookmarks : K     \r\n"
        " Configure : O     \r\n"
        " Back      : Esc   \r\n");

  if (root_status) {
    dputs("Toot:              \r\n"
          " Reply     : R     \r\n");
    if (root_status->spoiler_text) {
      dputs(" Toggle CW : W     \r\n");
    }
#if defined(__APPLE2ENH__) && !defined(IIGS)
    if (root_status->n_medias > 0) {
      dputs(" Medias    : I     \r\n");
    }
#else
    if (root_status->n_medias > 0 && root_status->media_type != MEDIA_TYPE_AUDIO) {
      dputs(" Medias    : I     \r\n");
    }
#endif
    if (root_status->poll) {
      dputs(" Vote      : V     \r\n");
    }
    if ((root_status->flags & FAVOURITED) != 0) {
      dputs(" Unfav.    : F     \r\n");
    } else {
      dputs(" Favourite : F     \r\n");
    }
    if ((root_status->flags & REBLOGGED) != 0) {
      dputs(" Unboost   : B     \r\n");
    } else {
      dputs(" Boost     : B     \r\n");
    }
    if ((root_status->flags & BOOKMARKED) != 0) {
      dputs(" Unbookmark: M     \r\n");
    } else {
      dputs(" Bookmark  : M     \r\n");
    }
    if (my_account && !strcmp(root_status->account->id, my_account->id)) {
      dputs(" Edit      : E     \r\n"
            " Delete    : D     \r\n");
    }
      dputs("Author:            \r\n"
            " Profile   : P     \r\n");
  } else if (l && l->account) {
      dputs("Profile:           \r\n"
            " Images    : I     \r\n");
    if (api_relationship_get(l->account, RSHIP_FOLLOWING)
     || api_relationship_get(l->account, RSHIP_FOLLOW_REQ)) {
      dputs(" Unfollow  : F     \r\n");
    } else {
      dputs(" Follow    : F     \r\n");
    }
    if (api_relationship_get(l->account, RSHIP_BLOCKING)) {
      dputs(" Unblock   : B     \r\n");
    } else {
      dputs(" Block     : B     \r\n");
    }
    if (api_relationship_get(l->account, RSHIP_MUTING)) {
      dputs(" Unmute    : M     \r\n");
    } else {
      dputs(" Mute      : M     \r\n");
    }
  } else if (root_notif) {
      dputs("Notifications:     \r\n"
            " All       : A     \r\n"
            " Mentions  : M     \r\n"
            "Profile:           \r\n"
            " Open      : P     \r\n");
  }
  dputs("Writing:           \r\n"
        " Compose   : C     \r\n");
#ifdef __CC65__
  while (wherey() < 23) {
    dputs("                   \r\n");
  }
#endif

  print_free_ram();
}

void __fastcall__ print_free_ram(void) {
#ifdef __APPLE2__
  gotoxy(0, 23);
  cprintf("%zuB free     ",
          _heapmemavail());
#endif
}
