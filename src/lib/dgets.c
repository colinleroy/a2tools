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

char * __fastcall__ dget_text(char *buf, size_t size, cmd_handler_func cmd_cb) {
#ifdef __CC65__
  char c;
  size_t i = 0, max_i = 0;
  int cur_x, cur_y;
  int prev_cursor = 0;
  unsigned char sx, wx;
  unsigned char sy, ey, hy;

  get_hscrollwindow(&sx, &wx);
  get_scrollwindow(&sy, &ey);
  hy = ey - sy;

  memset(buf, '\0', size - 1);
  prev_cursor = cursor(1);
  
  while (i < size - 1) {
    cur_x = wherex();
    cur_y = wherey();

    c = cgetc();

    if ((c & 0x80) != 0) {
      if (cmd_cb((c & ~0x80))) {
        goto out;
      }
    } else if (c == CH_ENTER) {
      dputc('\r');
      dputc('\n');
      buf[i] = c;
      i++;
      cur_x = 0;
    } else if (c == CH_ESC) {
      return NULL;
    } else if (c == CH_CURS_LEFT) {
      if (i > 0) {
        i--;
        cur_x--;
      }
      if (cur_x < 0) {
        cur_x = wx - 1;
        cur_y--;
      }
      gotoxy(cur_x, cur_y);
    } else if (c == CH_CURS_RIGHT) {
      if (i < max_i) {
        i++;
        cur_x++;
      }
      if (cur_x > wx - 1) {
        cur_x = 0;
        cur_y++;
      }
      gotoxy(cur_x, cur_y);
      if (cur_x > wx - 1) {
        cur_x = 0;
        cur_y++;
      }
      gotoxy(cur_x, cur_y);
    } else {
      dputc(c);
      buf[i] = c;
      i++;
      cur_x++;
    }
    if (i > max_i) {
      max_i = i;
    }    
  }
out:
  cursor(prev_cursor);
  buf[i] = '\0';

  return buf;
#else
  return fgets(buf, size, stdin);
#endif
}
