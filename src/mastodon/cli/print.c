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
#include "header.h"
#include "print.h"
#include "api.h"
#include "list.h"
#include "math.h"
#include "dgets.h"
#include "clrzone.h"
#include "scrollwindow.h"

int print_buf(char *w, char hide, char allow_scroll, char *scrolled) {
  char x = wherex(), y = wherey();
  while (*w) {
    if (allow_scroll && wherey() == scrh - 2) {
      gotoxy(0, scrh-1);
      dputs("Hit a key to continue.");
      cgetc();
      gotoxy(0, scrh-1);
      dputs("                      ");
      scrollup_n(10);
      y = scrh - 12;
      gotoxy(x, y);
      *scrolled = 1;
    }
    if (*w == '\n') {
      FAST_CHECK_AND_CRLF();
    } else {
      if (x == scrw - LEFT_COL_WIDTH - 2) {
        y++;
        x = 0;
        /* don't scroll last char */
        if (y == scrh) {
          cputc(hide ? '.':*w);
          return -1;
        }
      } else {
        x++;
      }
      if (!hide || *w == ' ' || *w == '\r')
        dputc(*w);
      else
        dputc('.');
    }
    ++w;
  }
  return 0;
}

int print_status(status *s, char hide, char full, char *scrolled) {
  char disp_idx, y;
  *scrolled = 0;
  y = disp_idx = wherey();
  s->displayed_at = disp_idx;
  /* reblog header */
  if (s->reblogged_by) {
    dputs(s->reblogged_by);
    dputs(" boosted");
    FAST_CHECK_AND_CRLF();
    s->displayed_at = disp_idx;
  }
  
  /* Display name + date */
  dputs(s->account->display_name); 
  dputs(" - ");
  dputs(s->created_at); 
  FAST_CHECK_AND_CRLF();

  /* username (30 chars max)*/
  dputc(arobase);
  dputs(s->account->username);
  
  FAST_CHECK_AND_CRLF();
  if (s->spoiler_text) {
    dputs("CW: ");
    dputs(s->spoiler_text);
    FAST_CHECK_AND_CRLF();
  }
  /* Content */
  if (print_buf(s->content, hide && s->spoiler_text != NULL, (full && disp_idx == 0), scrolled) < 0)
    return -1;
  y = wherey();
  FAST_CHECK_AND_CRLF();

  /* stats */
  FAST_CHECK_AND_CRLF();
  cprintf("%d replies, %s%d boosts, %s%d favs, %1d images      ",
        s->n_replies,
        (s->favorited_or_reblogged & REBLOGGED) ? "*":"", s->n_reblogs,
        (s->favorited_or_reblogged & FAVOURITED) ? "*":"", s->n_favourites,
        s->n_images);
  FAST_CHECK_AND_CRLF();

  chline(scrw - LEFT_COL_WIDTH - 2);
  FAST_CHECK_AND_CRLF();

  return 0;
}
