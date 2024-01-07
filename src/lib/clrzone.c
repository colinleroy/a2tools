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

#ifdef __CC65__
#pragma static-locals(push, on)
#endif

void __fastcall__ clrzone(char xs, char ys, char xe, char ye) {
#ifdef __APPLE2__
  /* Load by stack order */
  unsigned char lxs_even = xs;
  unsigned char lys = ys;
  unsigned char lxe_even = xe;
  unsigned char lye = ye;
  unsigned char lxs_odd, lxe_odd;

  /* Add WNDTOP */
  __asm__("clc");
  __asm__("lda "WNDTOP);
  __asm__("adc %v", lys);
  __asm__("sta %v", lys);
  /* Save start Y */
  __asm__("pha");

  __asm__("lda "WNDTOP);
  __asm__("adc %v", lye);
  __asm__("adc #1");
  __asm__("sta %v", lye);

  /* Are we in 80col? */
  __asm__("bit "RD80VID);
  __asm__("bmi %g", setup_80_bounds);

  /* Fix boundaries for 40col */
  __asm__("lda %v", lxs_even);
  __asm__("sta %v", lxs_odd);
  __asm__("pha"); /* save for gotoxy */
  __asm__("lda %v", lxe_even);
  __asm__("sec");
  __asm__("sbc %v", lxs_odd);
  __asm__("clc");
  __asm__("adc #1");
  __asm__("sta %v", lxe_odd);
  __asm__("lda #$00");
  __asm__("sta %v", lxs_even);
  __asm__("sta %v", lxe_even);
  __asm__("beq %g", start_clearing);

setup_80_bounds:
  /* Fix even/odd boundaries if 80col */
  __asm__("lda %v", lxs_even);
  __asm__("pha"); /* save for gotoxy */
  __asm__("lsr");
  __asm__("sta %v", lxs_odd);
  __asm__("bcc %g", lxs_is_even);
  __asm__("adc #0");
lxs_is_even:
  __asm__("sta %v", lxs_even);

  __asm__("clc");
  __asm__("lda %v", lxe_even);
  __asm__("adc #1");
  __asm__("lsr");
  __asm__("sta %v", lxe_odd);
  __asm__("bcc %g", lxe_is_even);
  __asm__("adc #0");
lxe_is_even:
  __asm__("sec");
  __asm__("sbc %v", lxs_even);
  __asm__("sta %v", lxe_even);

  __asm__("lda %v", lxe_odd);
  __asm__("sec");
  __asm__("sbc %v", lxs_odd);
  __asm__("sta %v", lxe_odd);

start_clearing:
  /* Start clearing */
  __asm__("lda %v", lys);
  __asm__("sta "CV);

next_line:
  __asm__("jsr FVTABZ");
  __asm__("pha"); /* save BASL */

  __asm__("bit "RD80VID);
  __asm__("bpl %g", do_low); /* no, just do low */

  __asm__("clc");
  __asm__("adc %v", lxs_even);
  __asm__("sta "BASL);

  __asm__("lda #' '|$80");

  __asm__("bit $C055");
  __asm__("ldy %v", lxe_even);
  __asm__("beq %g", do_low);
next_char_hi:
  __asm__("dey");
  __asm__("sta ("BASL"),y");
  __asm__("bne %g", next_char_hi);

do_low:
  __asm__("pla");  /* Restore BASL */
  __asm__("clc");
  __asm__("adc %v", lxs_odd);
  __asm__("sta "BASL);

  __asm__("lda #' '|$80");
  __asm__("bit $C054");
  __asm__("ldy %v", lxe_odd);
  __asm__("beq %g", do_next_line);
next_char_low:
  __asm__("dey");
  __asm__("sta ("BASL"),y");
  __asm__("bne %g", next_char_low);

do_next_line:
  __asm__("inc "CV);
  __asm__("lda "CV);
  __asm__("cmp %v", lye);
  __asm__("bcc %g", next_line);

  /* Goto start of cleared zone */
  __asm__("pla");
  __asm__("sta "CH);
  __asm__("pla");
  __asm__("sta "CV);
  __asm__("jsr FVTABZ");

#else
  char l = xe - xs + 1;

  memset(clearbuf, ' ', l);
  clearbuf[l] = '\0';

  do {
    cputsxy(xs, ys, clearbuf);
  } while (++ys <= ye);
#endif
}
#ifdef __CC65__
#pragma static-locals(pop)
#endif
