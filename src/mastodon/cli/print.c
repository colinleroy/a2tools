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
#include "header.h"
#include "print.h"
#include "api.h"
#include "list.h"
#include "math.h"
#include "dgets.h"
#include "clrzone.h"
#include "scrollwindow.h"

extern char writable_lines;

int print_buf(char *buffer, char hide, char allow_scroll) {
  static char x;
  static char *w;
  static char scrolled;
  static char wrap_idx;

  x = wherex();
  w = buffer;
  wrap_idx = scrw - LEFT_COL_WIDTH - 2;
  scrolled = 0;

  while (*w) {
    if (allow_scroll && writable_lines == 1) {
      gotoxy(0, scrh-1);
      dputs("Hit a key to continue.");
      cgetc();
      gotoxy(0, scrh - 1);
      dputs("                      ");
      gotoxy(0, scrh - 1);
      writable_lines += 14;
      scrolled = 1;
    }

    if (*w == '\n') {
      CHECK_AND_CRLF();
      x = 0;
    } else {
      if (x == wrap_idx) {
        CHECK_NO_CRLF();
        x = 0;
        /* don't scroll last char */
        if (writable_lines == 1) {
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
  if (scrolled) {
    return -1;
  }
  return 0;
}

int print_status(status *s, char hide, char full) {
  s->displayed_at = wherey();
  /* reblog header */
  if (s->reblogged_by) {
    dputs(s->reblogged_by);
    dputs(" boosted");
    CHECK_AND_CRLF();
  }

  /* Display name + date */
  if (strlen(s->account->display_name) > 30) {
    s->account->display_name[30] = '\0';
  }
  dputs(s->account->display_name);
  gotox(TIME_COLUMN);
  if (writable_lines != 1)
    dputs(s->created_at);
  else
    cputs(s->created_at); /* no scrolling please */
  CHECK_NO_CRLF();

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
  if (print_buf(s->content, hide && s->spoiler_text != NULL, (full && s->displayed_at == 0)) < 0)
    return -1;
  CHECK_AND_CRLF();
  /* stats */
  CHECK_AND_CRLF();
  cprintf("%d replies, %s%d boosts, %s%d favs, %1d images %s",
        s->n_replies,
        (s->flags & REBLOGGED) ? "*":"", s->n_reblogs,
        (s->flags & FAVOURITED) ? "*":"", s->n_favourites,
        s->n_images,
        (s->flags & BOOKMARKED) ? " - bookmarked":"             ");
  CHECK_AND_CRLF();

  CHLINE_SAFE();

  return 0;
}
