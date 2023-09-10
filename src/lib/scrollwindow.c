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
#include <apple2enh.h>
#endif

#include "scrollwindow.h"

void __fastcall__ get_scrollwindow(unsigned char *top, unsigned char *bottom){
#ifdef __CC65__
  *top = get_wndtop();
  *bottom = get_wndbtm();
#else
  *top = 0;
  *bottom = 24;
#endif

}

void __fastcall__ set_scrollwindow(unsigned char top, unsigned char bottom) {
#ifdef __CC65__
  set_wndtop(top);
  set_wndbtm(bottom);
#endif
}

void __fastcall__ get_hscrollwindow(unsigned char *left, unsigned char *width){
#ifdef __CC65__
  *left = get_wndlft();
  *width = get_wndwdth();
#else
  *left = 0;
  *width = 80;
#endif

}

void __fastcall__ set_hscrollwindow(unsigned char left, unsigned char width) {
#ifdef __CC65__
  set_wndlft(left);
  set_wndwdth(width);
#endif
}
