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

#ifndef __APPLE2__
static char clearbuf[82];
#endif
void __fastcall__ clrzone(char xs, char ys, char xe, char ye) {
#ifdef __APPLE2__
  /* Load by stack order */
  unsigned char lxs = xs;
  unsigned char lys = ys;
  unsigned char new_wdth = xe - lxs + 1;
  unsigned char lye = ye;

  /* Backup and update scrollwindow boundaries */
  __asm__("lda "WNDBTM);
  __asm__("pha");
  __asm__("lda "WNDTOP);
  __asm__("pha");
  __asm__("clc");
  __asm__("adc %v", lys);
  __asm__("sta "WNDTOP);
  __asm__("pla");
  __asm__("pha");
  __asm__("clc");
  __asm__("adc %v", lye);
  __asm__("adc #1");
  __asm__("sta "WNDBTM);

  __asm__("lda "WNDLFT);
  __asm__("pha");
  __asm__("clc");
  __asm__("adc %v", lxs);
  __asm__("sta "WNDLFT);
  __asm__("lda "WNDWDTH);
  __asm__("pha");
  __asm__("lda %v", new_wdth);
  __asm__("sta "WNDWDTH);

  clrscr();
  
  /* Restore boundaries */
  __asm__("pla");
  __asm__("sta "WNDWDTH);
  __asm__("pla");
  __asm__("sta "WNDLFT);
  __asm__("pla");
  __asm__("sta "WNDTOP);
  __asm__("pla");
  __asm__("sta "WNDBTM);

  gotoxy(lxs, lys);
#else
  char l = xe - xs + 1;

  memset(clearbuf, ' ', l);
  clearbuf[l] = '\0';

  do {
    cputsxy(xs, ys, clearbuf);
  } while (++ys <= ye);
#endif
}
