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
#include "config.h"
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "extended_conio.h"
#include "dgets.h"
#include "dputc.h"
#include "dputs.h"
#include "clrzone.h"
#include "scroll.h"
#include "scrollwindow.h"
#include "runtime_once_clean.h"
#ifdef __APPLE2__
#include "hgr.h"
#endif
#include "path_helper.h"
#include "platform.h"
#include "splash.h"
#include "malloc0.h"
#include "videoplay.h"

char *url = NULL;

unsigned char scrw = 255, scrh = 255;

char search_str[80];

#pragma code-name(push, "LOWCODE")

static void backup_restore_logo(char *op) {
  FILE *fp = fopen("/RAM/LOGO.HGR", op);
  if (!fp) {
    return;
  }
  if (op[0] == 'w') {
    fwrite((char *)HGR_PAGE, 1, HGR_LEN, fp);
  } else {
    fread((char *)HGR_PAGE, 1, HGR_LEN, fp);
  }
  fclose(fp);
}

static void load_video(char *id) {
  sprintf((char *)BUF_1K_ADDR, "%s/api/v1/videos/%s?local=true", url, id);

  surl_start_request(SURL_METHOD_GET, (char *)BUF_1K_ADDR, NULL, 0);

  if (!surl_response_ok()) {
    clrscr();
    gotoxy(0, 20);
    printf("Error loading video: %d", surl_response_code());
    cgetc();
    return;
  }

  if (surl_get_json((char *)BUF_1K_ADDR, BUF_1K_SIZE, SURL_HTMLSTRIP_NONE, translit_charset,
                    ".formatStreams[]|select(.itag==\"18\").url") >= 0) {
    stream_url((char *)BUF_1K_ADDR);
  }
}

char **lines;
char n_lines;
char cur_line = 0;

static int search_results(void) {
  int len;
  char c;

  n_lines = strsplit_in_place((char *)BUF_8K_ADDR, '\n', &lines);
  if (n_lines % 4 != 0) {
    cputs("Search error\n");
    cgetc();
    return -1;
  }

  init_hgr(1);
  hgr_mixon();

display_result:
  clrscr();
  gotoxy(0, 20);
  printf("%s (%s)\nUploaded by %s\n\n%d/%d results", lines[cur_line], lines[cur_line+1], lines[cur_line+2],
         (cur_line/4)+1, n_lines/4);

  surl_start_request(SURL_METHOD_GET, lines[cur_line+3], NULL, 0);

  if (surl_response_ok()) {

    simple_serial_putc(SURL_CMD_HGR);
    simple_serial_putc(1); /* monochrome */
    simple_serial_putc(HGR_SCALE_FULL);

    if (simple_serial_getc() == SURL_ERROR_OK) {

      surl_read_with_barrier((char *)&len, 2);
      len = ntohs(len);

      if (len == HGR_LEN) {
        surl_read_with_barrier((char *)HGR_PAGE, HGR_LEN);
      }
    }
  }
  c = cgetc();
  switch (c) {
    case CH_ENTER:
      load_video(lines[cur_line+1]);
      /* relaunch search */
      return 0;
    case CH_ESC:
      init_text();
      return -1;
    case CH_CURS_LEFT:
      if (cur_line > 3) {
        cur_line -= 4;
      }
      break;
    case CH_CURS_RIGHT:
      if (cur_line + 4 < n_lines) {
        cur_line += 4;
      }
      break;
  }
  goto display_result;
}

static int search(void) {
  sprintf((char *)BUF_1K_ADDR, "%s/api/v1/search?type=video&q=%s", url, search_str);

  surl_start_request(SURL_METHOD_GET, (char *)BUF_1K_ADDR, NULL, 0);

  if (!surl_response_ok()) {
    return -1;
  }

  if (surl_get_json((char *)BUF_8K_ADDR, BUF_8K_SIZE, SURL_HTMLSTRIP_NONE, translit_charset,
                    ".[]|.title,.videoId,.author,(.videoThumbnails[]|select(.quality == \"medium\")|.url)") >= 0) {
    return search_results();
  }
  return -1;
}

#ifdef __CC65__
#pragma code-name (pop)
#pragma code-name (push, "RT_ONCE")
#endif

static char *do_setup(void) {
  clrscr();
  url = strdup("https://invidious.fdn.fr/");

  set_scrollwindow(0, scrh);
  #ifdef __APPLE2__
  init_text();
  #endif

  return url;
}

int main(void) {
  char *url = NULL;

#ifdef __APPLE2ENH__
  videomode(VIDEOMODE_80COL);
#endif
#ifdef __APPLE2__
  init_hgr(1);
  hgr_mixon();
#endif
#ifdef __APPLE2ENH__
  backup_restore_logo("w");
#endif
  screensize(&scrw, &scrh);
  set_scrollwindow(20, scrh);
  
  surl_ping();
  load_config();

  url = do_setup();
  printf("started; %zuB free\n", _heapmaxavail());

new_search:
  clrscr();
  cputs("Search: ");
  dget_text(search_str, 80, NULL, 0);
  cur_line = 0;
same_search:
  if (search() == 0) {
    goto same_search;
  }
  goto new_search;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
