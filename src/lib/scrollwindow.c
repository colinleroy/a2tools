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
#include "scrollwindow.h"

void get_scrollwindow(unsigned char *top, unsigned char *bottom){
#ifdef __CC65__
  static char t, b;

  __asm__("lda $22"); /* get WNDTOP */
  __asm__("sta %v", t);
  __asm__("lda $23"); /* get WNDBTM */
  __asm__("sta %v", b);
  
  *top = t;
  *bottom = b;
#else
  *top = 0;
  *bottom = 24;
#endif

}

void set_scrollwindow(unsigned char top, unsigned char bottom) {
#ifdef __CC65__
  static char t, b;
  t = top;
  b = bottom;

  if (top >= bottom || bottom > 24)
    return;

  __asm__("lda %v", t);
  __asm__("sta $22"); /* store WNDTOP */
  __asm__("lda %v", b);
  __asm__("sta $23"); /* store WNDBTM */

#endif
}

void get_hscrollwindow(unsigned char *left, unsigned char *width){
#ifdef __CC65__
  static char l, w;

  __asm__("lda $20"); /* get WNDLFT */
  __asm__("sta %v", l);
  __asm__("lda $21"); /* get WNDWDTH */
  __asm__("sta %v", w);
  
  *left = l;
  *width = w;
#else
  *left = 0;
  *width = 80;
#endif

}

void set_hscrollwindow(unsigned char left, unsigned char width) {
#ifdef __CC65__
  static char l, w;
  l = left;
  w = width;

  if (l >= 79)
    return;

  __asm__("lda %v", l);
  __asm__("sta $20"); /* store WNDLFT */
  __asm__("lda %v", w);
  __asm__("sta $21"); /* store WNDWDTH */

#endif
}
