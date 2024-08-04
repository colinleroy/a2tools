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


#define SEARCH_API_ENDPOINT         "/api/v1/search?type=video&sort=relevance&q=%s"
#define VIDEO_DETAILS_JSON_SELECTOR ".[]|.title,.videoId,.author,(.videoThumbnails[]|select(.quality == \"medium\")|.url)"
#define VIDEO_DETAILS_ENDPOINT      "/api/v1/videos/%s?local=true"
#define VIDEO_URL_JSON_SELECTOR     ".formatStreams[]|select(.itag==\"18\").url"

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

static void load_indicator(char on) {
  gotoxy(76,3);
  cputs(on ? "...":"   ");
}

static void load_video(char *id) {
  load_indicator(1);
  sprintf((char *)BUF_1K_ADDR, "%s" VIDEO_DETAILS_ENDPOINT, url, id);

  surl_start_request(SURL_METHOD_GET, (char *)BUF_1K_ADDR, NULL, 0);

  if (!surl_response_ok()) {
    clrscr();
    printf("Error loading video: %d", surl_response_code());
    cgetc();
    goto out;
  }

  if (surl_get_json((char *)BUF_1K_ADDR, BUF_1K_SIZE, SURL_HTMLSTRIP_NONE, translit_charset,
                    VIDEO_URL_JSON_SELECTOR) >= 0) {
    load_indicator(0);
    stream_url((char *)BUF_1K_ADDR);

    backup_restore_logo("r");
    videomode(VIDEOMODE_80COL);
    set_scrollwindow(20, scrh);
    init_hgr(1);
    hgr_mixon();
    clrscr();
  }
out:
  load_indicator(0);
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
  printf("%s\nUploaded by %s\n\n%d/%d results", lines[cur_line], lines[cur_line+2],
         (cur_line/4)+1, n_lines/4);

  load_indicator(1);
  bzero((char *)HGR_PAGE, HGR_LEN);
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
  load_indicator(0);

  c = cgetc();
  switch (c) {
    case CH_ENTER:
      load_video(lines[cur_line+1]);
      /* relaunch search */
      return 0;
    case CH_ESC:
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
  sprintf((char *)BUF_1K_ADDR, "%s" SEARCH_API_ENDPOINT, url, search_str);

  load_indicator(1);
  surl_start_request(SURL_METHOD_GET, (char *)BUF_1K_ADDR, NULL, 0);

  if (!surl_response_ok()) {
    printf("Error %d\n", surl_response_code());
    cgetc();
    goto out;
  }

  if (surl_get_json((char *)BUF_8K_ADDR, BUF_8K_SIZE, SURL_HTMLSTRIP_NONE, translit_charset,
                    VIDEO_DETAILS_JSON_SELECTOR) >= 0) {
    load_indicator(0);
    return search_results();
  }
out:
  load_indicator(0);
  return -1;
}

#ifdef __CC65__
#pragma code-name (pop)
#pragma code-name (push, "RT_ONCE")
#endif

static void do_setup_url(char *op) {
  FILE *fp = fopen("STPSTARTURL", op);
  if (!fp) {
    return;
  }
  if (op[0] == 'w') {
    fputs(url, fp);
  } else {
    fgets((char *)BUF_1K_ADDR, BUF_1K_SIZE, fp);
    if(strchr((char *)BUF_1K_ADDR,'\n'))
      *strchr((char *)BUF_1K_ADDR,'\n') = '\0';
    url = strdup((char *)BUF_1K_ADDR);
  }
  fclose(fp);
}


static void do_setup(void) {
  clrscr();
  do_setup_url("r");

  if (url == NULL)
    url = strdup("https://invidious.fdn.fr/");

  cputs("Instance URL: ");
  dget_text((char *)BUF_1K_ADDR, BUF_1K_SIZE, NULL, 0);
  free(url);
  url = strdup((char *)BUF_1K_ADDR);
  do_setup_url("w");
}

int main(void) {
  videomode(VIDEOMODE_80COL);
  init_hgr(1);
  hgr_mixon();
  backup_restore_logo("w");

  screensize(&scrw, &scrh);
  set_scrollwindow(20, scrh);
  
  surl_ping();
  load_config();

  do_setup();
  printf("started; %zuB free\n", _heapmaxavail());

new_search:
  clrscr();
  cputs("Search videos: ");
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
