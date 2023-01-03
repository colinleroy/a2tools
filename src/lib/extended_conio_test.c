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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "extended_conio.h"
#include "bool_array.h"

char s_xlen[255], s_ylen[255];

int main(void) {
  int x = 0, y = 0;
  
  while (1) {
    cgetc();
    printfat(x, y, 1, "%d,%d-Printing at.", x, y);
    x++;
    y++;
    if (y > 22)
      y = 22;
  clrzone(1,1,7,22);

  }

  return (0);
}
