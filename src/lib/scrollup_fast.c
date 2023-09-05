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
#ifdef __CC65__
#include <conio.h>
#else
#include "extended_conio.h"
#endif
#include "clrzone.h"
#include "scrollwindow.h"
#include "surl.h"

static char scrlbuf[80];
static unsigned char wndlft, wndwdth, wndtop, wndbtm;

void __fastcall__ scrollup_fast (char count) {
  static char line;
  static char i;
  get_scrollwindow(&wndtop, &wndbtm);
  get_hscrollwindow(&wndlft, &wndwdth);

  line = wndtop + count;

  do {
    i = 0;
    gotoxy(0, line);
    while (i < wndwdth) {
      scrlbuf[i] = cpeekc();
      gotox(++i);
    }
    scrlbuf[i] = 0;

    gotoxy(0, line - count);

    cputs(scrlbuf);
  } while (++line < wndbtm);
  clrzone(0, line - count, wndwdth - 1, wndbtm - 1);
}
