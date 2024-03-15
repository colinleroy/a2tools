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
#include "stp_list.h"
#include "stp_cli.h"
#include "stp_save.h"
#include "stp_send_file.h"
#include "stp_delete.h"
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "dgets.h"
#include "dputc.h"
#include "dputs.h"
#include "clrzone.h"
#include "scroll.h"
#include "scrollwindow.h"
#include "strsplit.h"
#include "runtime_once_clean.h"

unsigned char scrw = 255, scrh = 255;
char center_x = 30; /* 12 in 40COLS */

char *welcome_header = NULL;

char data[STP_DATA_SIZE];
char **lines = NULL;

#ifdef __CC65__
#pragma code-name (push, "LC")
#endif

static void get_all(const char *url, char **lines, int n_lines) {
  int i, r;
  char *out_dir;
  if (lines == NULL || (out_dir = stp_confirm_save_all()) == NULL) {
    return;
  }
  for (i = 0; i < n_lines; i++) {
    const surl_response *resp = NULL;
    char *cur_url = strdup(url);
    cur_url = stp_url_enter(cur_url, lines[i]);
    resp = surl_start_request(SURL_METHOD_GET, cur_url, NULL, 0);

    stp_print_result(resp);

    gotoxy(0, 2);

    if (resp->size == 0) {
      free(cur_url);
      continue;
    }

    if (resp->content_type && strcmp(resp->content_type, "directory")) {
      r = stp_save_dialog(cur_url, resp, out_dir);
      stp_print_result(resp);
    } else {
      r = -1;
    }
    free(cur_url);
    if (r != 0) {
      break;
    }
  }
  clrzone(0, PAGE_BEGIN, scrw - 1, PAGE_BEGIN + PAGE_HEIGHT);
  free(out_dir);
}

void stp_print_footer(void) {
  gotoxy(0, 22);
  chline(scrw);
  clrzone(0, 23, scrw - 1, 23);
  gotoxy(0, 23);
  dputs("Up/Down, Enter: nav, S: send (R: recursive), D: delete, A: get all, Esc: back");
}

void stp_print_result(const surl_response *response) {
  gotoxy(0, 20);
  chline(scrw);
  clrzone(0, 21, scrw - 1, 21);
  gotoxy(0, 21);
  if (response == NULL) {
    dputs("Unknown request error.");
  } else {
    cprintf("Response code %d - %lu bytes",
            response->code,
            response->size);
  }
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

  url = stp_get_start_url("Please enter the server's root URL.", "ftp://ftp.apple.asimov.net/");
  url = stp_build_login_url(url);

  stp_print_footer();
  surl_set_time();

  runtime_once_clean();

  while(1) {
    switch (stp_get_data(url, &resp)) {
      case KEYBOARD_INPUT:
        goto keyb_input;
      case SAVE_DIALOG:
        stp_save_dialog(url, resp, NULL);
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
      case 'a':
      case 'A':
        get_all(url, lines, num_lines);
        break;
      case CH_CURS_UP:
        full_update = stp_list_scroll(-1);
        goto update_list;
      case CH_CURS_DOWN:
        full_update = stp_list_scroll(+1);
        goto update_list;
      case 's':
      case 'S':
        stp_send_file(url, 0);
        full_update = 1;
        break;
      case 'r':
      case 'R':
        stp_send_file(url, 1);
        full_update = 1;
        break;
      case 'd':
      case 'D':
        if (lines)
          stp_delete_dialog(url, lines[cur_line]);
        full_update = 1;
        break;
      default:
        goto update_list;
    }
  }

  exit(0);
}
#ifdef __CC65__
#pragma code-name (pop)
#endif
