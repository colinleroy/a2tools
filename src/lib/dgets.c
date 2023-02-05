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

static char echo_on = 1;
void echo(int on) {
  echo_on = on;
}

static char start_x, start_y;

static int get_prev_line_len(char *buf, size_t i, unsigned char wx) {
  int back;
  int prev_line_len;

  back = i - 1;

  while (back >= 0 && buf[back] != '\n') {
    --back;
  }

  ++back;
  if (back == 0) {
    back -= start_x;
  }
  prev_line_len = i - back;
  /* if it is a long line, only print its end */
  prev_line_len = prev_line_len % wx;
  return prev_line_len;
}

static void rewrite_start_of_buffer(char *buf, size_t i, unsigned char wx) {
  int prev_line_len, k;

  prev_line_len = get_prev_line_len(buf, i, wx);
  prev_line_len -= start_x;
  /* print it */
  gotoxy(start_x, start_y);
  for (k = i - prev_line_len; k <= i; k++) {
    cputc(buf[k]);
  }
}

static char rewrite_end_of_buffer(char *buf, size_t i, size_t max_i, unsigned char wx, unsigned char hy) {
  size_t k;
  unsigned char x, y;
  char overflowed;
  
  overflowed = 0;
  if (i == max_i) {
    /* Just clear EOL */
    x = wherex();
    y = wherey();
    clrzone(x, y, wx - 1, y);
    return 0;
  }
  for (k = i; k < max_i; k++) {
    x = wherex();
    y = wherey();
    if (buf[k] == '\n' || k == max_i - 1) {
      clrzone(x, y, wx - 1, y);
      gotoxy(x, y);
    }
    if (x == wx || k == max_i - 1) {
      if (y + 1 < hy) {
        clrzone(0, y + 1, wx - 1, y + 1);
        gotoxy(x, y);
      }
    }
    if (buf[k] == '\n') {
      cputc('\r');
    }
    y = wherey();
    cputc(buf[k]);
    if (y == hy - 1 && wherey() == 0) {
      /* overflowed bottom */
      overflowed = 1;
      break;
    }
  }

  return overflowed;
}

char * __fastcall__ dget_text(char *buf, size_t size, cmd_handler_func cmd_cb) {
#ifdef __CC65__
  char c;
  size_t i = 0, max_i = 0, k;
  int cur_x, cur_y;
  int prev_cursor = 0;
  unsigned char sx, wx;
  unsigned char sy, ey, hy, tmp;
  char scrolled_up = 0, overflowed = 0;

  get_hscrollwindow(&sx, &wx);
  get_scrollwindow(&sy, &ey);
  start_x = wherex();
  start_y = wherey();

  hy = ey - sy;

  memset(buf, '\0', size - 1);
  prev_cursor = cursor(1);

  while (1) {
    cur_x = wherex();
    cur_y = wherey();

    c = cgetc();

    if (cmd_cb && (c & 0x80) != 0) {
      if (cmd_cb((c & ~0x80))) {
        goto out;
      }
    } else if (c == CH_ESC) {
      if (cmd_cb)
        continue;
      else {
        max_i = 0;
        goto out;
      }
    } else if (c == CH_ENTER && !cmd_cb) {
      goto out;
    } else if (c == CH_CURS_LEFT || c == CH_DELETE) {
      if (i > 0) {
        i--;
        cur_x--;
        if (cur_x < 0) {
          cur_y--;
          cur_x = get_prev_line_len(buf, i, wx);
          if (cur_y < 0) {
            scrolldn();
            cur_y++;
            rewrite_start_of_buffer(buf, i, wx);
            gotoxy(cur_x, cur_y);
          }
        }
        if (c == CH_DELETE) {
          gotoxy(cur_x, cur_y);
          for (k = i; k < max_i; k++) {
            buf[k] = buf[k + 1];
          }
          max_i--;
          rewrite_end_of_buffer(buf, i, max_i, wx, hy);
        }
      } else {
        dputc(0x07);
      }
      gotoxy(cur_x, cur_y);
    } else if (c == CH_CURS_RIGHT) {
      if (i < max_i) {
        if (buf[i] != '\n') {
          /* We're not at end of line */
          cur_x++;
        } else {
          cur_y++;
          cur_x = 0;
        }
        i++;
        if (cur_x > wx - 1) {
          cur_x = 0;
          cur_y++;
        }
        /* Handle scroll up if needed */
        gotoxy(cur_x, cur_y);
        if (cur_x > wx - 1) {
          cur_x = 0;
          cur_y++;
          gotoxy(cur_x, cur_y);
        }
        scrolled_up = 0;
        while (cur_y > hy - 1) {
          cur_y--;
          scrollup();
          gotoxy(cur_x, cur_y);
          scrolled_up = 1;
        }
        if (scrolled_up) {
          rewrite_end_of_buffer(buf, i, max_i, wx, hy);
          gotoxy(cur_x, cur_y);
        }
      } else {
        dputc(0x07);
      }
    } else if (c == CH_CURS_UP) {
      if (!cmd_cb) {
        dputc(0x07);
      } else if (i == cur_x) {
        dputc(0x07);
      } else {
        i -= cur_x + 1;
        cur_y--;
        /* Decompose because rewrite_start_of_buffer
         * expects us to be on a \n */
        if (cur_y < 0) {
          scrolldn();
          cur_y++;
          rewrite_start_of_buffer(buf, i, wx);
        }
        if (buf[i] == '\n') {
          /* we're going to previous line */
          tmp = get_prev_line_len(buf, i, wx);
          if (tmp < cur_x) {
            cur_x = tmp;
          } else {
            i -= tmp - cur_x;
          }
        } else {
          /* going up in long line */
          i -= wx - cur_x - 1;
        }
        gotoxy(cur_x, cur_y);
      }
    } else if (c == CH_CURS_DOWN) {
      dputc(0x07);
      /* maybe we'll handle that later */
    } else {
      if (i == size - 1) {
        dputc(0x07);
        continue;
      }
      if (i < max_i) {
        /* Insertion. Use cputc to avoid autoscroll there */
        if (c == CH_ENTER) {
          /* Clear to end of line */
          clrzone(cur_x, cur_y, wx - 1, cur_y);
          if (cur_y + 1 < hy - 1) {
            /* Clear next line */
            clrzone(0, cur_y + 1, wx - 1, cur_y + 1);
          } else {
            /* we're on last line, scrollup */
            cur_y--;
            scrollup();
          }
          gotoxy(cur_x, cur_y);
          cputc('\r');
          cputc('\n');
        } else {
          /* advance cursor */
          dputc(echo_on ? c : '*');
        }
        /* insert char */
        if (max_i < size - 1)
          max_i++;
        for (k = max_i - 2; k >= i; k--) {
          buf[k + 1] = buf[k];
        }

        overflowed = rewrite_end_of_buffer(buf, i + 1, max_i, wx, hy);
        if (cur_y == hy - 1 && overflowed) {
          cur_y--;
          scrollup();
        }
        gotoxy(cur_x, cur_y);
        /* put back inserted char for cursor
         * advance again */
      }
      if (c == CH_ENTER) {
        dputc('\r');
        dputc('\n');
        buf[i] = '\n';
        i++;
        cur_x = 0;
      } else {
        dputc(echo_on ? c : '*');
        buf[i] = c;
        i++;
        cur_x++;
      }
    }
    if (i > max_i) {
      max_i = i;
    }    
  }
out:
  cursor(prev_cursor);
  buf[max_i] = '\0';

  if (!cmd_cb) {
    dputc('\r');
    dputc('\n');
  }
  return buf;
#else
  return fgets(buf, size, stdin);
#endif
}
