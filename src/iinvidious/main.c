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
#ifdef __APPLE2__
#include "hgr.h"
#endif
#include "path_helper.h"
#include "platform.h"
#include "splash.h"
#include "malloc0.h"
#include "videoplay.h"

enum InstanceType {
  PEERTUBE,
  INVIDIOUS,
  N_INSTANCE_TYPES
};

static const char *CHECK_API_ENDPOINT[] = {
  "/api/v1/config/about",
  "/api/v1/trending"
};

#define PEERTUBE_SEARCH_API_ENDPOINT         "/api/v1/search/videos?sort=-match&search=%s"
#define PEERTUBE_VIDEO_DETAILS_API_ENDPOINT  "/api/v1/videos/%s"
#define PEERTUBE_CAPTIONS_API_ENDPOINT       "/api/v1/videos/%s/captions"

#define INVIDIOUS_SEARCH_API_ENDPOINT         "/api/v1/search?type=video&sort=relevance&q=%s"
#define INVIDIOUS_VIDEO_DETAILS_API_ENDPOINT  "/api/v1/videos/%s?local=true"
#define INVIDIOUS_CAPTIONS_API_ENDPOINT       "/api/v1/captions/%s"

/* Peertube's .previewPath is an absolute URL without domain, but this can be
 * not addressed as the thumbnail request comes right after the video details
 * request, so the CURL handle proxy-side is set on the correct host already.
 */
static const char *VIDEO_DETAILS_JSON_SELECTOR[] = {
  ".data[]|.name,.uuid,.account.displayName,.previewPath,.duration",
  ".[]|.title,.videoId,.author,(.videoThumbnails[]|select(.quality == \"medium\")|.url),.lengthSeconds"
};

static const char *CAPTIONS_JSON_SELECTOR[] = {
  "(.data[]|select(.language.id == \"LG\")|.captionPath),.data[0].captionPath",
  "(.captions[]|select(.lang==\"LG\")|.url),.captions[0].url"
};

#define N_VIDEO_DETAILS 5
#define VIDEO_NAME   0
#define VIDEO_ID     1
#define VIDEO_AUTHOR 2
#define VIDEO_THUMB  3
#define VIDEO_LENGTH 4

static const char *VIDEO_URL_JSON_SELECTOR[] = {
  "[.files+.streamingPlaylists[0].files|.[]|select(.resolution.id > 0)]|sort_by(.size)|first|.fileDownloadUrl",
  ".formatStreams[]|select(.itag==\"18\").url"
};

char *url = NULL;
char instance_type = INVIDIOUS;

unsigned char scrw = 255, scrh = 255;

char search_str[80] = "";

#pragma code-name(push, "LC")

static void load_indicator(char on) {
  gotoxy(76,3);
  cputs(on ? "...":"   ");
}

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

static char did_cmd = 0;
static char cmd_cb(char c) {
  switch(tolower(c)) {
    case 'c':
      set_scrollwindow(0, 19);
      clrscr();
      init_text();
      config();
      set_scrollwindow(20, scrh);
      init_hgr(1);
      hgr_mixon();
      did_cmd = 1;
      return -1;
    case 'q':
      exit(0);
  }
  return -1;
}

#pragma code-name(pop)
#pragma code-name(push, "LOWCODE")

static void load_video(char *id) {
  strcpy(tmp_buf, id); /* Make it safe, id points to BUF_8K */
  load_indicator(1);
  if (instance_type == PEERTUBE)
    sprintf((char *)BUF_1K_ADDR, "%s" PEERTUBE_VIDEO_DETAILS_API_ENDPOINT, url, tmp_buf);
  else
    sprintf((char *)BUF_1K_ADDR, "%s" INVIDIOUS_VIDEO_DETAILS_API_ENDPOINT, url, tmp_buf);

  surl_start_request(SURL_METHOD_GET, (char *)BUF_1K_ADDR, NULL, 0);

  if (!surl_response_ok()) {
    clrscr();
    printf("Error %d", surl_response_code());
    cgetc();
    goto out;
  }

  if (surl_get_json((char *)BUF_8K_ADDR, BUF_8K_SIZE, SURL_HTMLSTRIP_NONE, translit_charset,
                    VIDEO_URL_JSON_SELECTOR[instance_type]) > 0) {
    char *captions = NULL;

    if (enable_subtitles) {
      if (instance_type == PEERTUBE)
        sprintf((char *)BUF_1K_ADDR, "%s" PEERTUBE_CAPTIONS_API_ENDPOINT, url, tmp_buf);
      else
        sprintf((char *)BUF_1K_ADDR, "%s" INVIDIOUS_CAPTIONS_API_ENDPOINT, url, tmp_buf);

      surl_start_request(SURL_METHOD_GET, (char *)BUF_1K_ADDR, NULL, 0);

      if (surl_response_ok()) {
        int url_len = strlen(url);
        char *sel;
        /* Prefix first result with instance URL */
        strcpy((char *)BUF_1K_ADDR, url);
        
        /* Update preferred subtitle language */
        sel = strstr(CAPTIONS_JSON_SELECTOR[instance_type], "LG");
        memcpy(sel, sub_language, 2);

        /* Get JSON right after the instance URL */
        if (surl_get_json((char *)(BUF_1K_ADDR + url_len), BUF_1K_SIZE - url_len,
                          SURL_HTMLSTRIP_NONE, translit_charset,
                          CAPTIONS_JSON_SELECTOR[instance_type]) > 0) {
          char *eol = strchr((char *)BUF_1K_ADDR, '\n');
          /* Cut at end of first match */
          if (eol) {
            *eol = '\0';
          }
          /* If we had an absolute URL without host */
          if (((char *)BUF_1K_ADDR)[url_len] == '/') {
            /* Pass our built URL */
            captions = (char *)BUF_1K_ADDR;
          } else {
            /* Pass what the server returned */
            captions = (char *)(BUF_1K_ADDR + url_len);
          }
        }
        /* Put language placeholder back */
        memcpy(sel, "LG", 2);
      }

    }
    load_indicator(0);
    stream_url((char *)BUF_8K_ADDR, captions);

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

char **lines = NULL;
char n_lines;
char cur_line = 0;

static void load_save_search_json(char *mode) {
  FILE *fp = fopen("/RAM/IINVSRCH", mode);
  if (!fp) {
    bzero((char *)BUF_8K_ADDR, BUF_8K_SIZE);
    return;
  }

  if (mode[0] == 'r') {
    fread((char *)BUF_8K_ADDR, 1, BUF_8K_SIZE, fp);
  } else {
    fwrite((char *)BUF_8K_ADDR, 1, BUF_8K_SIZE, fp);
  }
  fclose(fp);
}

static void search_results(void) {
  int len;
  char c;

reload_search:
  load_save_search_json("r");
  if (lines) {
    free(lines);
    lines = NULL;
  }
  n_lines = strsplit_in_place((char *)BUF_8K_ADDR, '\n', &lines);
  if (n_lines % 5 != 0) {
    cputs("Search error\n");
    cgetc();
    return;
  }

  init_hgr(1);
  hgr_mixon();

display_result:
  clrscr();
  len = atoi(lines[cur_line+VIDEO_LENGTH]);
  printf("%s\n"
         "%dm%ds - Uploaded by %s\n"
         "\n"
         "%d/%d results",
         lines[cur_line+VIDEO_NAME],
         len/60, len%60,
         lines[cur_line+VIDEO_AUTHOR],
         (cur_line/N_VIDEO_DETAILS)+1, n_lines/N_VIDEO_DETAILS);

  load_indicator(1);
  bzero((char *)HGR_PAGE, HGR_LEN);
  surl_start_request(SURL_METHOD_GET, lines[cur_line+3], NULL, 0);

  if (surl_response_ok()) {

    simple_serial_putc(SURL_CMD_HGR);
    simple_serial_putc(1); /* monochrome */
    simple_serial_putc(HGR_SCALE_MIXHGR);

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
      goto reload_search;
    case CH_ESC:
      return;
    case CH_CURS_LEFT:
      if (cur_line > N_VIDEO_DETAILS-1) {
        cur_line -= N_VIDEO_DETAILS;
      }
      break;
    case CH_CURS_RIGHT:
      if (cur_line + N_VIDEO_DETAILS < n_lines) {
        cur_line += N_VIDEO_DETAILS;
      }
      break;
  }
  goto display_result;
}


static int search(void) {
  if (instance_type == PEERTUBE)
    sprintf((char *)BUF_1K_ADDR, "%s" PEERTUBE_SEARCH_API_ENDPOINT, url, search_str);
  else
    sprintf((char *)BUF_1K_ADDR, "%s" INVIDIOUS_SEARCH_API_ENDPOINT, url, search_str);

  load_indicator(1);
  surl_start_request(SURL_METHOD_GET, (char *)BUF_1K_ADDR, NULL, 0);

  if (!surl_response_ok()) {
    clrscr();
    printf("Error %d", surl_response_code());
    load_indicator(0);
    cgetc();
  } else {
    if (surl_get_json((char *)BUF_8K_ADDR, BUF_8K_SIZE, SURL_HTMLSTRIP_NONE, translit_charset,
                      VIDEO_DETAILS_JSON_SELECTOR[instance_type]) > 0) {
      load_indicator(0);
      load_save_search_json("w");
      search_results();
    } else {
      clrscr();
      printf("No results.");
      load_indicator(0);
      cgetc();
    }
  }
}

static void do_ui(void) {
new_search:
  clrscr();
  gotoxy(0, 3);
  cputc('A'|0x80);
  cputs("-C: Configure ; ");
  cputc('A'|0x80);
  cputs("-Q: Quit");
  printf(" - %zuB free", _heapmemavail());
  gotoxy(0, 0);
  cputs("Search videos: ");
  did_cmd = 0;
  dget_text(search_str, 80, cmd_cb, 0);
  if (did_cmd) {
    goto new_search;
  }
  cur_line = 0;
  search();
  backup_restore_logo("r");
  goto new_search;
}

#ifdef __CC65__
#pragma code-name (pop)
#pragma code-name (push, "RT_ONCE")
#endif

static int define_instance(void) {
  const surl_response *resp;
  char i;

  for (i = 0; i < N_INSTANCE_TYPES; i++) {
    sprintf((char *)BUF_1K_ADDR, "%s%s", url, CHECK_API_ENDPOINT[i]);

    resp = surl_start_request(SURL_METHOD_GET, (char *)BUF_1K_ADDR, NULL, 0);

    if (surl_response_ok() && resp->size > 0) {
      instance_type = i;
      return 0;
    }
  }

  return -1;
}

static void do_setup_url(char *op) {
  FILE *fp = fopen("STPSTARTURL", op);
  if (!fp) {
    return;
  }
  if (op[0] == 'w') {
    fputs(url, fp);
  } else {
    fgets((char *)BUF_1K_ADDR, BUF_1K_SIZE, fp);
    if(strchr((char *)BUF_1K_ADDR,'\n')) {
      *strchr((char *)BUF_1K_ADDR,'\n') = '\0';
    }
    url = strdup((char *)BUF_1K_ADDR);
  }
  fclose(fp);
}


static void do_setup(void) {
  char modified;

  do_setup_url("r");

  if (url == NULL)
    url = strdup("https://invidious.fdn.fr");

again:
  clrscr();
  printf("Free memory: %zuB\n", _heapmemavail());
  cputs("Instance URL: ");
  strcpy((char *)BUF_1K_ADDR, url);
  dget_text((char *)BUF_1K_ADDR, BUF_1K_SIZE, NULL, 0);
  modified = strcmp((char *)BUF_1K_ADDR, url) != 0;
  free(url);
  url = strdup((char *)BUF_1K_ADDR);
  if (url[strlen(url)-1] == '/') {
    url[strlen(url)-1] = '\0';
  }
  if (define_instance() < 0) {
    cputs("Could not identify instance type.");
    cgetc();
    goto again;
  }
  if (modified) {
    do_setup_url("w");
  }
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

  do_ui();
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
