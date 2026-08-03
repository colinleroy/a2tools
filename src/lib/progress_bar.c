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
#include "progress_bar.h"

#pragma inline-stdfuncs(push, on)
#pragma allow-eager-inline(push, on)
#pragma codesize(push, 200)
void __fastcall__ progress_bar(signed char x, signed char y, unsigned char width, unsigned long cur, unsigned long end) {
  size_t percent;
  size_t i;
  static size_t last_percent;
  static unsigned char last_x, last_y;

#ifndef __CC65__
  return;
#endif

  if (x >= 0) {
    gotoxy(x, y);
    last_percent = 0;
  } else {
    gotoxy(last_x, last_y);
  }

  percent = (size_t)(cur * width / end);
  if (percent > width) {
    percent = width;
  }

  revers(1);
  for (i = last_percent; i < percent; i++)
    cputc(' ');
  revers(0);

  last_percent = percent;
  last_x = wherex();
  last_y = wherey();

  if (x >= 0) {
    for (; i < width; i++)
      cputc(0x7F);
    gotoxy(x, y);
  }
}
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)
