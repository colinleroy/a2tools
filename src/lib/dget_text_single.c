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
#include "a2_features.h"

#ifdef __CC65__
#pragma optimize(push, on)
#pragma static-locals(push, on)
#pragma code-name (push, "LOWCODE")
#endif

char dgets_echo_on = 1;

static unsigned char win_width_min1;
static size_t cur_insert, max_insert;
static char *text_buf;

#ifndef __CC65__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

static void __fastcall__ rewrite_end_of_buffer(void) {
  size_t k;
  unsigned char x, y;

  if (cur_insert == max_insert) {
    /* Just clear EOL */
    clreol();
    return;
  }

  k = cur_insert;
  goto initxy;

  while (k < max_insert) {
    char c = text_buf[k];
    if (k == max_insert - 1) {
      clreol();
      if (x == win_width_min1) {
        gotoxy(0, y + 1);
        clreol();
        gotoxy(x, y);
      }
    }

    cputc(c);

    k++;
initxy:
    x = wherex();
    y = wherey();
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
  size_t k;

  cur_insert = 0;
  max_insert = 0;
  text_buf = buf;

  get_hscrollwindow(&sx, &win_width_min1);
  win_width_min1--;

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

    c = oa_cgetc();
    if (is_iie && cmd_cb && (c & 0x80) != 0) {
      if (cmd_cb((c & ~0x80))) {
        goto out;
      }
      gotoxy(cur_x, cur_y);
    /* No Open-Apple there, let's do it with Ctrl */
    } else if (!is_iie && cmd_cb && c < 27 &&
        c != CH_ENTER && c != CH_CURS_LEFT && c != CH_CURS_RIGHT) {
      if (cmd_cb(c + 'A' - 1)) {
        goto out;
      }
      gotoxy(cur_x, cur_y);
    } else if (c == CH_ESC) {
      max_insert = 0;
      goto out;
    } else if (c == CH_ENTER) {
      goto out;
    } else if (c == CH_CURS_LEFT
       || (is_iie && c == CH_DEL)
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
        cur_x = win_width_min1;
        cur_y--;
      } else {
        cur_x--;
      }
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
      gotoxy(cur_x, cur_y);
    } else if (c == CH_CURS_RIGHT) {
      /* are we at buffer end? */
      if (cur_insert == max_insert) {
        goto err_beep;
      }
      cur_x++;

      /* Are we at end of soft line? */
      if (cur_x > win_width_min1) {
        /* We are, go down and left */
        cur_y++;
        cur_x = 0;
      }

      cur_insert++;

      /* Handle scroll up if needed */
      gotoxy(cur_x, cur_y);
    } else if (c == CH_CURS_UP) {
      goto err_beep;
    } else if (c == CH_CURS_DOWN) {
      goto err_beep;
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

        if (is_iie) { /* No insertion on Apple II+ */
          /* shift end of buffer */
          k = max_insert;
          max_insert++;
          while (k != cur_insert) {
            k--;
            text_buf[k + 1] = text_buf[k];
          }
        }

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

  if (!is_iie) {
    /* No deletion on non-enhanced Apple 2 so remove everything
     * after the cursor */
    max_insert = cur_insert;
  }

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
