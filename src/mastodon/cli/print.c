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
  while (*w) {
    if (allow_scroll && wherey() == scrh - 2) {
      char i;
      gotoxy(0, scrh-1);
      dputs("Hit a key to continue.");
      cgetc();
      gotoxy(0, scrh-1);
      dputs("                      ");
      for (i = 0; i < 10; i++) {
        scrollup();
      }
      gotoxy(0, scrh - 12);
      *scrolled = 1;
    }
    if (*w == '\n') {
      CHECK_AND_CRLF();
    } else {
      if (wherey() == scrh - 1 && wherex() == scrw - LEFT_COL_WIDTH - 2) {
        return -1;
      }
      if (!hide || *w == ' ' || *w == '\r' || *w == '\n')
        dputc(*w);
      else
        dputc('.');
    }
    ++w;
  }
  return 0;
}

int print_status(status *s, char hide, char full, char *scrolled) {
  char disp_idx;
  *scrolled = 0;
  disp_idx = wherey();
  s->displayed_at = disp_idx;
  /* reblog header */
  if (s->reblog) {
    dputs(s->account->display_name);
    dputs(" boosted");
    CHECK_AND_CRLF();
    s = s->reblog;
    s->displayed_at = disp_idx;
  }
  
  /* Display name + date */
  dputs(s->account->display_name); 
  dputs(" - ");
  dputs(s->created_at); 
  CHECK_AND_CRLF();

  /* username (30 chars max)*/
  dputc(arobase);
  dputs(s->account->username);
  
  CHECK_AND_CRLF();
  if (s->spoiler_text) {
    dputs("CW: ");
    dputs(s->spoiler_text);
    CHECK_AND_CRLF();
  }
  /* Content */
  if (print_buf(s->content, hide && s->spoiler_text != NULL, (full && disp_idx == 0), scrolled) < 0)
    return -1;

  CHECK_AND_CRLF();
  if (wherey() == scrh - 1) {
    return -1;
  }

  /* stats */
  CHECK_AND_CRLF();
  cprintf("%d replies, %s%d boosts, %s%d favs, %1d images      ",
        s->n_replies,
        (s->favorited_or_reblogged & REBLOGGED) ? "*":"", s->n_reblogs,
        (s->favorited_or_reblogged & FAVOURITED) ? "*":"", s->n_favourites,
        s->n_images);
  CHECK_AND_CRLF();
  if (wherey() == scrh - 1) {
    return -1;
  }

  chline(scrw - LEFT_COL_WIDTH - 1);
  if (wherey() == scrh - 1) {
    return -1;
  }

  return 0;
}
