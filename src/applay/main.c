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
#include <unistd.h>
#include <errno.h>
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "dgets.h"
#include "dputc.h"
#include "dputs.h"
#include "clrzone.h"
#include "scroll.h"
#include "scrollwindow.h"
#include "stp_list.h"
#include "runtime_once_clean.h"
#include "hgr.h"

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

unsigned char scrw = 255, scrh = 255;

void stp_print_footer(void) {
  gotoxy(0, 22);
  chline(scrw);
  clrzone(0, 23, scrw - 1, 23);
  gotoxy(0, 23);
  dputs("Up/Down, Enter: nav, Esc: back");
}

int main(void) {
  char *url;
  char c;
  char full_update = 1;
  const surl_response *resp;

  videomode(VIDEOMODE_80COL);

  clrscr();
  screensize(&scrw, &scrh);

  surl_ping();
  clrscr();

  url = stp_get_start_url();
  url = stp_build_login_url(url);

  stp_print_footer();

  runtime_once_clean();

  while(1) {
    switch (stp_get_data(url, &resp)) {
      case KEYBOARD_INPUT:
        goto keyb_input;
      case SAVE_DIALOG:
        /* Play */
        clrzone(0, PAGE_BEGIN, scrw - 1, PAGE_BEGIN + PAGE_HEIGHT);
        stp_print_result(resp);
        goto up_dir;
      case UPDATE_LIST:
      default:
        break;
    }

update_list:
    stp_update_list(full_update);

keyb_input:
    c = cgetc();
    switch(c) {
      case CH_ESC:
up_dir:
        url = stp_url_up(url);
        full_update = 1;
        break;
      case CH_ENTER:
        if (lines)
          url = stp_url_enter(url, lines[cur_line]);
        full_update = 1;
        break;
      case CH_CURS_UP:
        full_update = stp_list_scroll(-1);
        goto update_list;
      case CH_CURS_DOWN:
        full_update = stp_list_scroll(+1);
        goto update_list;
      default:
        goto update_list;
    }
    free(data);
    data = NULL;
    free(lines);
    lines = NULL;
  }

  exit(0);
}
#ifdef __CC65__
#pragma code-name (pop)
#endif
