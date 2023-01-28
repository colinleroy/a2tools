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

static char *clearbuf = NULL;
void __fastcall__ clrzone(char xs, char ys, char xe, char ye) {
  int l = xe - xs + 1;
  
  if (clearbuf == NULL) {
    clearbuf = malloc(80 + 2);
    memset(clearbuf, ' ', 80 + 2);
  }

  memset(clearbuf, ' ', l);
  clearbuf[l] = '\0';

  while (ys < ye + 1) {
    cputsxy(xs, ys, clearbuf);
    ys++;
  }
}