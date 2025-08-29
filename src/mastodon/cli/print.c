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
#include "clrzone.h"
#include "header.h"
#include "print.h"
#include "api.h"
#include "list.h"
#include "math.h"
#include "progress_bar.h"
#include "scrollwindow.h"
#include "a2_features.h"

extern char writable_lines;
static char wrap_idx;

#pragma register-vars(push, on)

#define CH      "$24"
#define OURCH   "$057B"
void __fastcall__ clrnln(void) {
  clreol();
  #ifdef __APPLE2__
    __asm__("lda #0");
    __asm__("bit %v", has_80cols);
    __asm__("bpl %g", set40);
    __asm__("sta "OURCH);
    set40:
    __asm__("sta "CH);
    __asm__("lda #$0A");
    __asm__("jsr _dputc");
  #endif
}

int print_buf(register char *buffer, char hide, char allow_scroll) {
  static char x;
  register char *w;
  static char l_allow_scroll;
  static char l_hide;
  static char scrolled;

  x = wherex();
  w = buffer;
  l_hide = hide;
  l_allow_scroll = allow_scroll;

  wrap_idx = RIGHT_COL_AWIDTH;
  scrolled = 0;

  while (*w) {
    if (l_allow_scroll && writable_lines == 1) {
      gotoxy(0, NUMLINES - 1);
      dputs("Hit a key to continue.");
      cgetc();

      gotoxy(0, NUMLINES - 1);
      dputs("                      ");
      gotoxy(0, NUMLINES - 1);
      writable_lines += 14;
      scrolled = 1;
      x = 0;
    }

    if (*w == '\n') {
      CHECK_AND_CRLF();
      x = 0;
    } else {
      if (x == wrap_idx) {
        --writable_lines;
        x = 0;

        /* don't scroll last char */
        if (writable_lines == 0 && !l_allow_scroll) {
          cputc(l_hide ? '.':*w);
          return -1;
        }
      } else {
        x++;
      }
      if (!l_hide || *w == ' ' || *w == '\r') {
        dputc(*w);
      } else {
        dputc('.');
      }
    }
    ++w;
    /* Skip start of line spaces */
    while (!x && *w == ' ') {
      w++;
    }
  }
  if (scrolled) {
    return -1;
  }
  return 0;
}

const char *stats_str = "%d replies, %s%d boosts, %s%d favs, %1d %ss %s";

int print_status(register status *s, char hide, char full) {
  poll *p = s->poll;
  account *a = s->account;
  char scrolled = 0;

  s->displayed_at = wherey();
  /* reblog header */
  if (IS_NOT_NULL(s->reblogged_by)) {
    if (!has_80cols && strlen(s->reblogged_by) > 30) {
      s->reblogged_by[30] = '\0';
    }
    dputs(s->reblogged_by);
    dputs(" boosted");
    CHECK_AND_CRLF();
  }

  /* Display name + date */
  dputs(a->display_name);

  if (has_80cols) {
    clreol();
    gotox(TIME_COLUMN);
    if (writable_lines != 1)
      dputs(s->created_at);
    else
      cputs(s->created_at); /* no scrolling please */
    CHECK_NO_CRLF();
  } else {
    CHECK_AND_CRLF();
  }

  /* username (30 chars max)*/
  dputc(arobase);
  dputs(a->username);

  CHECK_AND_CRLF();

  if (IS_NOT_NULL(s->spoiler_text)) {
    if (!has_80cols && strlen(s->spoiler_text) > 30) {
      s->spoiler_text[30] = '\0';
    }
    dputs("CW: ");
    dputs(s->spoiler_text);
    CHECK_AND_CRLF();
  }
  /* Content */
  scrolled = print_buf(s->content, hide && IS_NOT_NULL(s->spoiler_text), (full && s->displayed_at == 0));
  if (scrolled && !(full && s->displayed_at == 0))
    return -1;
  if (IS_NOT_NULL(s->quote)) {
      status *q = s->quote;

      CHECK_AND_CRLF();
      dputs("_Quote: ");
      dputs(q->account->display_name ? q->account->display_name : q->account->acct);
      dputs("___");
      CHECK_AND_CRLF();
      dputc(CH_VLINE);
      scrolled = print_buf(q->content, hide && IS_NOT_NULL(q->spoiler_text), (full && s->displayed_at == 0));
      if (wherex() < RIGHT_COL_WIDTH - 3) {
        dputc('_');
        dputc(CH_VLINE);
      }
      if (scrolled && !(full && s->displayed_at == 0))
        return -1;
  }
  CHECK_AND_CRLF();

  if (IS_NOT_NULL(p)) {
    char i;
    size_t total = p->votes_count;

    if (total == 0) {
      total = 1;
    }

    for (i = 0; i < p->options_count; i++) {
      poll_option *o = &(p->options[i]);
      CHECK_AND_CRLF();
      dputs(p->own_votes[i] == 1 ? "(*) ":"( ) ");
      dputs(o->title);
      CHECK_AND_CRLF();
      dputs("    ");
      progress_bar(wherex(), wherey(), wrap_idx - 4,
                   o->votes_count, total);
      gotox(wrap_idx); /* don't clear our progress_bar */
      CHECK_AND_CRLF();
    }
  }
  /* stats */
  CHECK_AND_CRLF();

  if (!has_80cols) {
    stats_str = "%d rep, %s%d boost, %s%d fav, %1d %s %s";
  }
  cprintf(stats_str,
      s->n_replies,
      (s->flags & REBLOGGED) ? "*":"", s->n_reblogs,
      (s->flags & FAVOURITED) ? "*":"", s->n_favourites,
      s->n_medias, media_type_str[s->media_type],
      (s->flags & BOOKMARKED) ? (has_80cols?" - bookmarked":"-bkm"):"");

  CHECK_AND_CRLF();

  CHLINE_SAFE();

  return scrolled;
}

#pragma register-vars(pop)
