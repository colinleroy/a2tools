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
#include <ctype.h>
#include "stp_list.h"
#include "stp_cli.h"
#include "stp_save.h"
#include "stp_send_file.h"
#include "stp_create_dir.h"
#include "stp_delete.h"
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "dputc.h"
#include "clrzone.h"
#include "scroll.h"
#include "scrollwindow.h"
#include "strsplit.h"
#include "runtime_once_clean.h"
#include "charsets.h"
#include "vsdrive.h"
#include "a2_features.h"

#define CENTERX_40COLS 12
#define CENTERX_80COLS 30

unsigned char center_x;
unsigned char NUMCOLS;

char *translit_charset = US_CHARSET;

char large_buf[STP_DATA_SIZE];
char *data = (char *)large_buf;
char **lines = NULL;
char *nat_data = NULL;
unsigned int max_nat_data_size = 0;
unsigned char nat_data_static = 0;
char **nat_lines = NULL;
extern char **display_lines;

#ifdef __CC65__
#pragma code-name (push, "LC")
#endif

static void get_all(const char *url, char **lines, int n_lines) {
  int i, r;
  char *out_dir;
  extern surl_response resp;

  if (lines == NULL || (out_dir = stp_confirm_save_all()) == NULL) {
    return;
  }
  for (i = 0; i < n_lines; i++) {
    char *cur_url = strdup(url);
    cur_url = stp_url_enter(cur_url, lines[i]);
    stp_print_header(display_lines[i], URL_ADD);
    surl_start_request(NULL, 0, cur_url, SURL_METHOD_GET);

    stp_print_result();

    gotoxy(0, 2);

    if (resp.size == 0) {
      free(cur_url);
      continue;
    }

    if (resp.content_type && strcmp(resp.content_type, "directory")) {
      r = stp_save_dialog(cur_url, out_dir);
      stp_print_result();
    } else {
      r = -1;
    }
    stp_print_header(NULL, URL_UP);
    free(cur_url);
    if (r != 0) {
      break;
    }
  }
  clrzone(0, PAGE_BEGIN, NUMCOLS - 1, PAGE_BEGIN + PAGE_HEIGHT);
  free(out_dir);
}

void stp_print_footer(void) {
  gotoxy(0, 22);
  chline(NUMCOLS);
  clrzone(0, 23, NUMCOLS - 1, 23);
  gotoxy(0, 23);

  if(has_80cols) {
    cputs("Up/Down/Ret/Esc:nav, S:send (R:all), D:del, A:get all, /:Search");
    if (search_buf[0]) {
      cputs(", N:Next");
    } else {
      cputs(", M:mkdir");
    }
    cputs(", Q:Quit");
  } else if (is_iie) {
    cputs("Up/Down/Ret/Esc:nav, S:send, Q:quit");
  } else {
    cputs("U/J/Ret/Esc:nav, S:send, Q:quit");
  }
}

void stp_print_result(void) {
  extern surl_response resp;
  clrzone(0, 21, NUMCOLS - 1, 21);
  gotoxy(0, 21);
  cprintf("Response code %d - %lu bytes",
          surl_response_code(),
          resp.size);
}

static void init_ui_width(void) {
    if (has_80cols) {
      center_x = CENTERX_80COLS;
      NUMCOLS = 80;
    } else {
      center_x = CENTERX_40COLS;
      NUMCOLS = 40;
    }
}

int main(void) {
  char *url;
  char c, l;
  char full_update = 1;

  /* init bufs */
  search_buf[0] = '\0';
  tmp_buf[0] = '\0';

  try_videomode(VIDEOMODE_80COL);

  init_ui_width();

  clrscr();

  surl_ping();
  surl_user_agent = "STP "VERSION"/Apple II";

  clrscr();
  gotoxy(0, 14);
  url = stp_get_start_url("Please enter the server's root URL.\r\n\r\n",
                          "ftp://ftp.apple.asimov.net/", NULL);
  url = stp_build_login_url(url);
  stp_print_header(url, URL_SET);

  surl_set_time();

  vsdrive_install();

  runtime_once_clean();

  while(1) {
    switch (stp_get_data(url)) {
      case KEYBOARD_INPUT:
        goto keyb_input;
      case SAVE_DIALOG:
        stp_save_dialog(url, NULL);
        clrzone(0, PAGE_BEGIN, NUMCOLS - 1, PAGE_BEGIN + PAGE_HEIGHT);
        stp_print_result();
        goto up_dir;
      case UPDATE_LIST:
      default:
        break;
    }

update_list:
    stp_update_list(full_update);

keyb_input:
    while (!kbhit()) {
      stp_animate_list(0);
    }
    c = tolower(cgetc());
    l = (c & 0x80) ? PAGE_HEIGHT : 1;
    full_update = 0;

    switch(c & ~0x80) {
      case CH_ESC:
up_dir:
        url = stp_url_up(url);
        stp_print_header(NULL, URL_UP);
        full_update = 1;
        break;
      case CH_ENTER:
        if (lines) {
          url = stp_url_enter(url, lines[cur_line]);
          stp_print_header(display_lines[cur_line], URL_ADD);
        }
        full_update = 1;
        break;
      case 'a':
        get_all(url, lines, num_lines);
        full_update = 1;
        break;
      case CH_CURS_UP:
      case 'u':
        while (l--)
          full_update |= stp_list_scroll(-1);
        goto update_list;
      case CH_CURS_DOWN:
      case 'j':
        while (l--)
          full_update |= stp_list_scroll(+1);
        goto update_list;
      case 's':
        stp_send_file(url, 0);
        full_update = 1;
        break;
      case 'r':
        stp_send_file(url, 1);
        full_update = 1;
        break;
      case 'm':
        stp_create_dir(url);
        full_update = 1;
        break;
      case 'd':
        if (lines)
          stp_delete_dialog(url, lines[cur_line]);
        full_update = 1;
        break;
      case '/':
      case 'n':
        stp_list_search((c == '/'));
        stp_print_header(NULL, URL_RESTORE);
        goto keyb_input;
      case 'q':
        goto quit;
      default:
        goto update_list;
    }
  }
quit:
  exit(0);
}
#ifdef __CC65__
#pragma code-name (pop)
#endif
