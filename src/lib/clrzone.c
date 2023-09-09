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
#include "extended_conio.h"
#include "scrollwindow.h"
#include "clrzone.h"

#ifndef __APPLE2ENH__
static char clearbuf[82];
#endif
void __fastcall__ clrzone(char xs, char ys, char xe, char ye) {
#ifdef __APPLE2ENH__
  unsigned char orig_top, orig_bottom;
  unsigned char orig_left, orig_width;

  get_scrollwindow(&orig_top, &orig_bottom);
  get_hscrollwindow(&orig_left, &orig_width);

  xs += orig_left;
  ys += orig_top;
  set_scrollwindow(ys, ye + 1);
  set_hscrollwindow(xs, xe - xs + 1);
  clrscr();
  set_hscrollwindow(orig_left, orig_width);
  set_scrollwindow(orig_top, orig_bottom);

#else
  char l = xe - xs + 1;

  memset(clearbuf, ' ', l);
  clearbuf[l] = '\0';

  do {
    cputsxy(xs, ys, clearbuf);
  } while (++ys <= ye);
#endif
}
