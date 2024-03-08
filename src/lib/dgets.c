/*
 * Copyright (C) 2022 Colin Leroy-Mira <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include "dgets.h"
#include "extended_conio.h"
#include "dputc.h"
#include "clrzone.h"
#include "scrollwindow.h"
#include "scroll.h"
#include "surl.h"

#ifdef __CC65__
#pragma optimize(push, on)
#pragma static-locals(push, on)
#pragma code-name (push, "LOWCODE")
#endif

static char echo_on = 1;
void __fastcall__ echo(char on) {
  echo_on = on;
}

static char start_x, start_y;
static unsigned char win_width, win_height;
static unsigned char win_width_min1, win_height_min1;
static size_t cur_insert, max_insert;
static char *text_buf;

static char __fastcall__ get_prev_line_len() {
  int back;
  char prev_line_len;

  back = cur_insert - 1;

  while (back >= 0 && text_buf[back] != '\n') {
    --back;
  }

  ++back;
  if (back == 0) {
    back -= start_x;
  }
  prev_line_len = (cur_insert - back) % win_width;
  return prev_line_len;
}

#ifndef __CC65__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

static void __fastcall__ scroll_down_and_rewrite_start_of_buffer() {
  char prev_line_len;
  int k;

  scrolldown_one();
  prev_line_len = get_prev_line_len() - start_x;
  /* print it */
  gotoxy(start_x, start_y);
  for (k = cur_insert - prev_line_len; k <= cur_insert; k++) {
    cputc(text_buf[k]);
  }
}

static char __fastcall__ rewrite_end_of_buffer(char full) {
  size_t k;
  unsigned char x, y, y_plus1;
  char overflowed;
  char first_crlf;

  overflowed = 0;
  first_crlf = !full;

  if (cur_insert == max_insert) {
    /* Just clear EOL */
    clreol();
    return 0;
  }

  k = cur_insert;
  goto initxy;

  while (k < max_insert) {
    char c = text_buf[k];
    if (c == '\n' || k == max_insert - 1) {
      clreol();
    }
    if (x == win_width || k == max_insert - 1) {
      if (y_plus1 < win_height) {
        gotoxy(0, y_plus1);
        clreol();
        gotoxy(x, y);
      }
    }
    if (c == '\n') {
      if (x != 0 && x != win_width_min1 && first_crlf) {
        /* we can stop there, we won't shift lines down or up */
        break;
      }
      first_crlf = 0;
      cputc('\r');
    }
    cputc(c);
    if (y == win_height_min1 && wherey() == 0) {
      /* overflowed bottom */
      overflowed = 1;
      break;
    }
    k++;
initxy:
    x = wherex();
    y = wherey();
    y_plus1 = y + 1;
  }
  return overflowed;
}

#ifndef __CC65__
#pragma GCC diagnostic pop
#endif

char * __fastcall__ dget_text(char *buf, size_t size, cmd_handler_func cmd_cb, char enter_accepted) {
  char c;
  char cur_x, cur_y;
#ifdef __CC65__
  char prev_cursor = 0;
#endif
  unsigned char sx;
  unsigned char sy, ey;
#ifdef __APPLE2ENH__
  size_t k;
#ifdef DGETS_MULTILINE
  unsigned char tmp;
#endif
#endif
  char overflowed = 0;

  cur_insert = 0;
  max_insert = 0;
  text_buf = buf;

  get_hscrollwindow(&sx, &win_width);
  get_scrollwindow(&sy, &ey);
  start_x = wherex();
  start_y = wherey();

  win_height = ey - sy;
  win_width_min1 = win_width - 1;
  win_height_min1 = win_height - 1;

  if (start_x + size < win_width && !enter_accepted) {
    /* We'll never handle another line */
    win_height = 1;
    win_height_min1 = 1;
  }

  if (text_buf[0] != '\0') {
    max_insert = strlen(text_buf);
    for (cur_insert = 0; cur_insert < max_insert; cur_insert++) {
      char c = text_buf[cur_insert];
      if (c != '\n') {
        dputc(c);
      } else {
        dputc('\r');
        dputc('\n');
      }
    }
  }
#ifdef __CC65__
  prev_cursor = cursor(1);
#endif
  while (1) {
    cur_x = wherex();
    cur_y = wherey();

    c = cgetc();

#ifdef __APPLE2ENH__
    if (cmd_cb && (c & 0x80) != 0) {
      if (cmd_cb((c & ~0x80))) {
        goto out;
      }
      gotoxy(cur_x, cur_y);
#else
    /* No Open-Apple there, let's do it with Ctrl */
    if (cmd_cb && c < 27 &&
        c != CH_ENTER && c != CH_CURS_LEFT && c != CH_CURS_RIGHT) {
      if (cmd_cb(c + 'A' - 1)) {
        goto out;
      }
      gotoxy(cur_x, cur_y);
#endif
    } else if (c == CH_ESC) {
      if (cmd_cb && enter_accepted)
        continue;
      else {
        max_insert = 0;
        goto out;
      }
    } else if (c == CH_ENTER && (!cmd_cb || !enter_accepted)) {
      goto out;
    } else if (c == CH_CURS_LEFT
#ifdef __APPLE2ENH__
       || c == CH_DEL
#endif
     ) {
      if (cur_insert == 0) {
err_beep:
        dputc(0x07);
        continue;
      }
      /* Go back one step in the buffer */
      cur_insert--;
      /* did we hit start of (soft) line ? */
      if (cur_x == 0) {
        /* recompute x */
        cur_x = get_prev_line_len();
        /* do we have to scroll (we were at line 0) ? */
        if (cur_y == 0) {
          scroll_down_and_rewrite_start_of_buffer();
        } else {
          /* go up */
          cur_y--;
        }
      } else {
        cur_x--;
      }
#ifdef __APPLE2ENH__
      if (c == CH_DEL) {
        char deleted = text_buf[cur_insert];
        /* shift chars down */
        for (k = cur_insert; k < max_insert; k++) {
          text_buf[k] = text_buf[k + 1];
        }
        /* dec length */
        max_insert--;
        /* update display */
        gotoxy(cur_x, cur_y);
        rewrite_end_of_buffer(deleted == '\n');
      }
#endif
      gotoxy(cur_x, cur_y);
    } else if (c == CH_CURS_RIGHT) {
      /* are we at buffer end? */
      if (cur_insert == max_insert) {
        goto err_beep;
      }
      /* Are we at end of hard line ? */
      if (text_buf[cur_insert] != '\n') {
        /* We're not at end of line, go right */
        cur_x++;
      } else {
        /* We are, go down and left */
        goto down_left;
      }

      /* Are we at end of soft line now? */
      if (cur_x > win_width_min1) {
        /* We are, go down and left */
down_left:
        cur_y++;
        cur_x = 0;
      }

      cur_insert++;

      /* Handle scroll up if needed */
      if (cur_y > win_height_min1) {
        cur_y--;
        scrollup_one();
        gotoxy(cur_x, cur_y);
        rewrite_end_of_buffer(0);
      }
      gotoxy(cur_x, cur_y);
#ifdef __APPLE2ENH__
    } else if (c == CH_CURS_UP) {
      #ifdef DGETS_MULTILINE
      if (!cmd_cb || !enter_accepted || cur_insert == 0) {
        /* No up/down in standard line edit */
        goto err_beep;
      }
      if (cur_insert == cur_x) {
        /* we are at the first line, go at its beginning */
        cur_x = 0;
        cur_insert = 0;
      } else {
        /* Go back in the buffer to the character just
         * before the current offset to left border */
        cur_insert -= cur_x + 1;
        /* and go up to previous line */
        /* Decompose because scroll_down_and_rewrite_start_of_buffer
         * expects us to be on a \n */
        if (cur_y == 0) {
          scroll_down_and_rewrite_start_of_buffer();
        } else {
          cur_y--;
        }

        /* are we at a  line hard end? */
        if (text_buf[cur_insert] == '\n') {
          /* we are, so don't go as far right as we were */
          tmp = get_prev_line_len();
          if (tmp < cur_x) {
            cur_x = tmp;
          } else {
            cur_insert -= tmp - cur_x;
          }
        } else {
          /* just going up in long line */
          cur_insert -= win_width_min1 - cur_x;
        }
      }
      gotoxy(cur_x, cur_y);
    #else
      goto err_beep;
    #endif
    } else if (c == CH_CURS_DOWN) {
      #ifdef DGETS_MULTILINE
      if (!cmd_cb || !enter_accepted || cur_insert == max_insert) {
        /* No down in standard editor mode */
        goto err_beep;
      }
      /* Save cur_x */
      tmp = cur_x;
      /* wrap to EOL, either hard or soft */
      while (cur_x < win_width_min1 && text_buf[cur_insert] != '\n') {
        cur_insert++;
        cur_x++;
        if (cur_insert == max_insert) {
          /* Can't go down, abort */
          goto stop_down;
        }
      }
      cur_insert++;
      cur_x = 0;

      /* Scroll if we need */
      if (cur_y == win_height_min1) {
        scrollup_one();
        gotoxy(cur_x, cur_y);
        rewrite_end_of_buffer(0);
      } else {
        cur_y++;
      }

      /* Advance to previous cur_x at most */
      while (cur_x < tmp) {
        if (cur_insert == max_insert || text_buf[cur_insert] == '\n') {
          break;
        }
        cur_insert++;
        cur_x++;
      }
stop_down:
      gotoxy(cur_x, cur_y);
    #else
      goto err_beep;
    #endif
#endif
    } else {
      if (max_insert == size) {
        /* Full buffer */
        goto err_beep;
      }
      if (cur_insert < max_insert) {
        /* Insertion in the middle of the buffer.
         * Use cputc to avoid autoscroll there */
        if (c == CH_ENTER) {
          /* Clear to end of line */
          clreol();
          /* Are we on the last line? */
          if (cur_y == win_height_min1) {
            /* we're on last line, scrollup */
            cur_y--;
            scrollup_one();
            gotoxy(cur_x, cur_y);
          }
          cputc('\r');
          cputc('\n');
        } else {
          /* advance cursor */
#ifdef __CC65__
          dputc(echo_on ? c : '*');
#endif
        }

#ifdef __APPLE2ENH__ /* No insertion on non-enhanced Apple 2 */
        /* shift end of buffer */
        k = max_insert;
        max_insert++;
        while (k != cur_insert) {
          k--;
          text_buf[k + 1] = text_buf[k];
        }
#endif

        /* rewrite buffer after inserted char */
        cur_insert++;
#ifdef __APPLE2ENH__
        overflowed = rewrite_end_of_buffer(c == CH_ENTER);
#else /* No insertion on non-enhanced Apple 2 so rewrite everything */
        overflowed = rewrite_end_of_buffer(1);
#endif
        cur_insert--;

        if (cur_y == win_height_min1 && overflowed) {
          cur_y--;
          scrollup_one();
          /* rewrite again for last line */
          gotoxy(cur_x, cur_y);
          rewrite_end_of_buffer(0);
        }
        gotoxy(cur_x, cur_y);
      }
      if (c == CH_ENTER) {
#ifdef __CC65__
        dputc('\r');
        dputc('\n');
#endif
        c = '\n';
      } else {
#ifdef __CC65__
        dputc(echo_on ? c : '*');
#endif
      }
      text_buf[cur_insert] = c;
      cur_insert++;
    }
    if (cur_insert > max_insert) {
      max_insert = cur_insert;
    }
  }
out:

#ifndef __APPLE2ENH__
  /* No deletion on non-enhanced Apple 2 so remove everything
   * after the cursor */
  max_insert = cur_insert;
#endif
  cursor(prev_cursor);
  text_buf[max_insert] = '\0';

  if (!cmd_cb) {
    dputc('\r');
    dputc('\n');
  }
  return text_buf;
}

#ifdef __CC65__
#pragma code-name (pop)
#pragma optimize(pop)
#endif
