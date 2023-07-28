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
#include "progress_bar.h"

void progress_bar(int x, int y, int width, size_t cur, size_t end) {
  unsigned long percent;
  unsigned int i;

  gotoxy(x, y);
  percent = (long)cur * (long)width;
  percent = percent/((long)end);
  revers(1);
  for (i = 0; i <= ((int)percent) && i < width; i++)
    cputc(' ');
  revers(0);
  for (i = (int)(percent + 1L); i < width; i++)
    cputc(0x7F);
}
