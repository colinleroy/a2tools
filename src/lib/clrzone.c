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

#ifdef __APPLE2__
static unsigned char clr_lxs_even;
static unsigned char clr_lys;
static unsigned char clr_lxe_even;
static unsigned char clr_lye;
static unsigned char clr_lxs_odd, clr_lxe_odd;
static unsigned char clr_x_bck, clr_y_bck;
#endif

static void __fastcall__ do_clear(void);

void __fastcall__ clreol(void) {
#ifdef __APPLE2__
  __asm__("lda "CH);
  __asm__("sta %v", clr_lxs_even);
  __asm__("clc");
  __asm__("lda "CV);
  __asm__("sta %v", clr_lys);
  __asm__("sta %v", clr_lye);
  __asm__("sta %v", clr_y_bck);
  __asm__("lda "WNDWDTH);
  __asm__("sec");
  __asm__("sbc #1");
  __asm__("sta %v", clr_lxe_even);
#endif
  do_clear();
}

void __fastcall__ clrzone(char xs, char ys, char xe, char ye) {
#ifdef __APPLE2__
  /* Load by stack order */
  clr_lxs_even = xs;
  clr_lys = ys;
  clr_lxe_even = xe;
  clr_lye = ye;

  /* Add WNDTOP */
  __asm__("clc");
  __asm__("lda "WNDTOP);
  __asm__("adc %v", clr_lys);
  __asm__("sta %v", clr_lys);
  /* Save start Y */
  __asm__("sta %v", clr_y_bck);

  __asm__("lda "WNDTOP);
  __asm__("adc %v", clr_lye);
  __asm__("adc #1");
  __asm__("sta %v", clr_lye);
  do_clear();
#else
  char l = xe - xs + 1;

  memset(clearbuf, ' ', l);
  clearbuf[l] = '\0';

  do {
    cputsxy(xs, ys, clearbuf);
  } while (++ys <= ye);
#endif
}

static void __fastcall__ do_clear(void) {
#ifdef __APPLE2__
  /* Are we in 80col? */
  __asm__("bit "RD80VID);
  __asm__("bmi %g", setup_80_bounds);

  /* Fix boundaries for 40col */
  __asm__("lda %v", clr_lxs_even);
  __asm__("sta %v", clr_lxs_odd);
  __asm__("sta %v", clr_x_bck); /* save for gotoxy */
  __asm__("lda %v", clr_lxe_even);
  __asm__("sec");
  __asm__("sbc %v", clr_lxs_odd);
  __asm__("clc");
  __asm__("adc #1");
  __asm__("sta %v", clr_lxe_odd);
  __asm__("lda #$00");
  __asm__("sta %v", clr_lxs_even);
  __asm__("sta %v", clr_lxe_even);
  __asm__("beq %g", start_clearing);

setup_80_bounds:
  /* Fix even/odd boundaries if 80col */
  __asm__("lda %v", clr_lxs_even);
  __asm__("sta %v", clr_x_bck); /* save for gotoxy */
  __asm__("lsr");
  __asm__("sta %v", clr_lxs_odd);
  __asm__("bcc %g", lxs_is_even);
  __asm__("adc #0");
lxs_is_even:
  __asm__("sta %v", clr_lxs_even);

  __asm__("clc");
  __asm__("lda %v", clr_lxe_even);
  __asm__("adc #1");
  __asm__("lsr");
  __asm__("sta %v", clr_lxe_odd);
  __asm__("bcc %g", lxe_is_even);
  __asm__("adc #0");
lxe_is_even:
  __asm__("sec");
  __asm__("sbc %v", clr_lxs_even);
  __asm__("sta %v", clr_lxe_even);

  __asm__("lda %v", clr_lxe_odd);
  __asm__("sec");
  __asm__("sbc %v", clr_lxs_odd);
  __asm__("sta %v", clr_lxe_odd);

start_clearing:
  /* Start clearing */
  __asm__("lda %v", clr_lys);
  __asm__("sta "CV);

next_line:
  __asm__("jsr FVTABZ");
  __asm__("pha"); /* save BASL */

  __asm__("bit "RD80VID);
  __asm__("bpl %g", do_low); /* no, just do low */

  __asm__("clc");
  __asm__("adc %v", clr_lxs_even);
  __asm__("sta "BASL);

  __asm__("lda #' '|$80");

  __asm__("bit $C055");
  __asm__("ldy %v", clr_lxe_even);
  __asm__("beq %g", do_low);
next_char_hi:
  __asm__("dey");
  __asm__("sta ("BASL"),y");
  __asm__("bne %g", next_char_hi);

do_low:
  __asm__("pla");  /* Restore BASL */
  __asm__("clc");
  __asm__("adc %v", clr_lxs_odd);
  __asm__("sta "BASL);

  __asm__("lda #' '|$80");
  __asm__("bit $C054");
  __asm__("ldy %v", clr_lxe_odd);
  __asm__("beq %g", do_next_line);
next_char_low:
  __asm__("dey");
  __asm__("sta ("BASL"),y");
  __asm__("bne %g", next_char_low);

do_next_line:
  __asm__("inc "CV);
  __asm__("lda "CV);
  __asm__("cmp %v", clr_lye);
  __asm__("bcc %g", next_line);

  /* Goto start of cleared zone */
  __asm__("lda %v", clr_x_bck);
  __asm__("sta "CH);
  __asm__("lda %v", clr_y_bck);
  __asm__("sta "CV);
  __asm__("jsr FVTABZ");
#endif
}
#ifdef __CC65__
#pragma static-locals(pop)
#endif
