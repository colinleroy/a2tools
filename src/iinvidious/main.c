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
#include "surl/surl_stream_av/stream_url.h"

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
#ifdef __APPLE2ENH__
  gotoxy(76, 3);
#else
  gotoxy(36, 3);
#endif
  cputs(on ? "...":"   ");
}

#ifdef __APPLE2ENH__
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
#endif

static char did_cmd = 0;
static char cmd_cb(char c) {
  char prev_cursor = cursor(0);
  switch(tolower(c)) {
    case 'c':
      set_scrollwindow(0, 19);
      clrscr();
      init_text();
      config();
      clrscr();
      set_scrollwindow(20, scrh);
      did_cmd = 1;
      break;
    case 'q':
      exit(0);
  }
  cursor(prev_cursor);
  return -1;
}

static void print_menu(void) {
#ifdef __APPLE2ENH__
  cputc('A'|0x80);
  cputs("-C: Configure ; ");
  cputc('A'|0x80);
  cputs("-Q: Quit");
  printf(" - %zuB free", _heapmemavail());
#else
  cputs("Ctrl-C: Configure; Ctrl-Q: Quit");
#endif
}

static void load_save_search_json(char *mode) {
#ifdef __APPLE2ENH__
  FILE *fp = fopen("/RAM/IINVSRCH", mode);
#else
  FILE *fp = fopen("IINVSRCH", mode);
#endif
  if (!fp) {
    if (mode[0] == 'r')
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

#pragma code-name(pop)
#pragma code-name(push, "LOWCODE")

static void load_video(char *id) {
  int url_len = strlen(url);
  char *video_url = NULL;
  char *captions_url = NULL;

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

  /* Prefix result with instance URL in case it has no http[s]://host,
   * and video URI into the very large buffer, because Youtube's videos
   * URIs are enormous */
  strcpy((char *)BUF_8K_ADDR, url);

  if (surl_get_json((char *)(BUF_8K_ADDR + url_len), BUF_8K_SIZE - url_len,
                    SURL_HTMLSTRIP_NONE, translit_charset,
                    VIDEO_URL_JSON_SELECTOR[instance_type]) > 0) {

    /* If we had an absolute URL without host */
    if (((char *)BUF_8K_ADDR)[url_len] == '/') {
      /* Pass our built URL */
      video_url = (char *)BUF_8K_ADDR;
    } else {
      /* Pass what the server returned */
      video_url = (char *)(BUF_8K_ADDR + url_len);
    }

    if (enable_subtitles) {
      if (instance_type == PEERTUBE)
        sprintf((char *)BUF_1K_ADDR, "%s" PEERTUBE_CAPTIONS_API_ENDPOINT, url, tmp_buf);
      else
        sprintf((char *)BUF_1K_ADDR, "%s" INVIDIOUS_CAPTIONS_API_ENDPOINT, url, tmp_buf);

      surl_start_request(SURL_METHOD_GET, (char *)BUF_1K_ADDR, NULL, 0);

      if (surl_response_ok()) {
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
            captions_url = (char *)BUF_1K_ADDR;
          } else {
            /* Pass what the server returned */
            captions_url = (char *)(BUF_1K_ADDR + url_len);
          }
        }
        /* Put language placeholder back */
        memcpy(sel, "LG", 2);
      }

    }
    load_indicator(0);
    stream_url(video_url, captions_url);
    set_scrollwindow(20, scrh);

#ifdef __APPLE2ENH__
    backup_restore_logo("r");
#endif
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

static void search_results(void) {
  int len;
  char c;

  load_save_search_json("w");
reload_search:
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

display_result:
  clrscr();
  len = atoi(lines[cur_line+VIDEO_LENGTH]);
#ifdef __APPLE2ENH__
  if (strlen(lines[cur_line+VIDEO_NAME]) > 78)
    lines[cur_line+VIDEO_NAME][78] = '\0';

  if (strlen(lines[cur_line+VIDEO_AUTHOR]) > 64)
    lines[cur_line+VIDEO_AUTHOR][64] = '\0';
#else
  if (strlen(lines[cur_line+VIDEO_NAME]) > 38)
    lines[cur_line+VIDEO_NAME][38] = '\0';

  if (strlen(lines[cur_line+VIDEO_AUTHOR]) > 24)
    lines[cur_line+VIDEO_AUTHOR][24] = '\0';
#endif

  printf("%s\n"
         "%dm%ds - Uploaded by %s\n"
         "%d/%d results\n",
         lines[cur_line+VIDEO_NAME],
         len/60, len%60,
         lines[cur_line+VIDEO_AUTHOR],
         (cur_line/N_VIDEO_DETAILS)+1, n_lines/N_VIDEO_DETAILS);
  print_menu();

  load_indicator(1);
  init_text();
  surl_start_request(SURL_METHOD_GET, lines[cur_line+VIDEO_THUMB], NULL, 0);
  bzero((char *)HGR_PAGE, HGR_LEN);
  if (surl_response_ok()) {
    #ifdef SIMPLE_SERIAL_DUMP
    simple_serial_dump('B', (char *)0x400, 0xBFFF-0x400);
    simple_serial_dump('B', (char *)0xD400, 0xDFFF-0xD400);
    #endif
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

read_kbd:
  init_hgr(1);
  hgr_mixon();
  c = cgetc();
#ifdef __APPLE2ENH__
  if (c & 0x80) {
    cmd_cb(c & ~0x80);
    goto read_kbd;
  }
#endif
  switch (c) {
#ifndef __APPLE2ENH__
    case 'C' - 'A' + 1:
    case 'Q' - 'A' + 1:
      cmd_cb(c + 'A' - 1);
      goto read_kbd;
#endif
    case CH_ENTER:
      load_video(lines[cur_line+1]);
      /* relaunch search */
      load_save_search_json("r");
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
      search_results();
    } else {
      clrscr();
      printf("No results.");
      load_indicator(0);
      cgetc();
    }
  }
}

#ifdef __CC65__
#pragma code-name (pop)
#endif

static void do_ui(void) {
new_search:
  clrscr();
  gotoxy(0, 3);
  print_menu();
  gotoxy(0, 0);
  cputs("Search videos: ");
  did_cmd = 0;
  dget_text(search_str, 80, cmd_cb, 0);
  if (did_cmd) {
    goto new_search;
  }
  cur_line = 0;
  search();
#ifdef __APPLE2ENH__
  backup_restore_logo("r");
#endif
  goto new_search;
}

#ifdef __CC65__
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
    fgets(tmp_buf, TMP_BUF_SIZE, fp);
    tmp_buf[TMP_BUF_SIZE-1] = '\0';
    if(strchr(tmp_buf,'\n')) {
      *strchr(tmp_buf,'\n') = '\0';
    }
    url = strdup(tmp_buf);
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
  dget_text((char *)BUF_1K_ADDR, 80, NULL, 0);
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
#ifdef __APPLE2ENH__
  videomode(VIDEOMODE_80COL);
  backup_restore_logo("w");
#endif

  screensize(&scrw, &scrh);
  surl_ping();

#ifdef __APPLE2__
  init_hgr(1);
  hgr_mixon();
  set_scrollwindow(20, scrh);
  clrscr();
#endif

  load_config();

  do_setup();
  printf("started; %zuB free\n", _heapmaxavail());

  #ifdef SIMPLE_SERIAL_DUMP
  simple_serial_dump('A', (char *)0x400, 0xBFFF-0x400);
  simple_serial_dump('A', (char *)0xD400, 0xDFFF-0xD400);
  #endif

  do_ui();
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
