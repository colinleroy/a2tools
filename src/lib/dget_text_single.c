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
#include "dget_text.h"
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

char dgets_echo_on = 1;

static char start_x, start_y;
static unsigned char win_width, win_height;
static unsigned char win_width_min1, win_height_min1;
static size_t cur_insert, max_insert;
static char *text_buf;

static char __fastcall__ get_prev_line_len(void) {
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

static void __fastcall__ rewrite_end_of_buffer(void) {
  size_t k;
  unsigned char x, y, y_plus1;

  if (cur_insert == max_insert) {
    /* Just clear EOL */
    clreol();
    return;
  }

  k = cur_insert;
  goto initxy;

  while (k < max_insert) {
    char c = text_buf[k];
    if (c == '\n' || k == max_insert - 1) {
      clreol();
    }

    /* Clear start of next line if we go back to a single line
     * even in not MULTILINE mode, we can wrap. */
    if (x == win_width || k == max_insert - 1) {
      if (y_plus1 < win_height) {
        gotoxy(0, y_plus1);
        clreol();
        gotoxy(x, y);
      }
    }

    cputc(c);

    k++;
initxy:
    x = wherex();
    y = wherey();
    y_plus1 = y + 1;
  }
}

#ifndef __CC65__
#pragma GCC diagnostic pop
#endif

char * __fastcall__ dget_text_single(char *buf, size_t size, cmd_handler_func cmd_cb) {
  char c;
  char cur_x, cur_y;
#ifdef __CC65__
  char prev_cursor = 0;
#endif
  unsigned char sx;
  unsigned char sy, ey;
#ifdef __APPLE2ENH__
  size_t k;
#endif

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

  win_height = 1;
  win_height_min1 = 1;

  if (text_buf[0] != '\0') {
    max_insert = strlen(text_buf);
    for (cur_insert = 0; cur_insert < max_insert; cur_insert++) {
      char c = text_buf[cur_insert];

      dputc(dgets_echo_on ? c : '*');
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
      max_insert = 0;
      goto out;
    } else if (c == CH_ENTER) {
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
        cur_y--;
      } else {
        cur_x--;
      }
#ifdef __APPLE2ENH__
      if (c == CH_DEL) {
        /* shift chars down */
        for (k = cur_insert; k < max_insert; k++) {
          text_buf[k] = text_buf[k + 1];
        }
        /* dec length */
        max_insert--;
        /* update display */
        gotoxy(cur_x, cur_y);
        rewrite_end_of_buffer();
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
      gotoxy(cur_x, cur_y);
#ifdef __APPLE2ENH__
    } else if (c == CH_CURS_UP) {
      goto err_beep;
    } else if (c == CH_CURS_DOWN) {
      goto err_beep;
#endif
    } else if (c == 0x09) {
      /* Tab */
      goto err_beep;
    } else {
      if (max_insert == size - 1) {
        /* Full buffer */
        goto err_beep;
      }
      if (cur_insert < max_insert) {
        /* Insertion in the middle of the buffer.
         * Use cputc to avoid autoscroll there */
        {
          /* advance cursor */
#ifdef __CC65__
          dputc(dgets_echo_on ? c : '*');
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
        rewrite_end_of_buffer();
        cur_insert--;

        gotoxy(cur_x, cur_y);
      }

#ifdef __CC65__
      dputc(dgets_echo_on ? c : '*');
#endif
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
