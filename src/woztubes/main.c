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
#include "citoa.h"
#include "video_providers.h"

char *url = NULL;

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
#define SEARCH_SAVE_FILE "/RAM/WTSRCH"
#define LOGO_SAVE_FILE "/RAM/LOGO.HGR"
#else
#define SEARCH_SAVE_FILE "WTSRCH"
#endif

#ifdef __APPLE2ENH__
static void backup_restore_logo(char *op) {
  FILE *fp = fopen(LOGO_SAVE_FILE, op);
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
#ifdef SEARCH_SAVE_FILE
      unlink(SEARCH_SAVE_FILE);
#endif
#ifdef LOGO_SAVE_FILE
      unlink(LOGO_SAVE_FILE);
#endif
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
  cputs("-Q: Quit - ");
  cutoa(_heapmemavail());
  cputs("B free");
#else
  cputs("Ctrl-C: Configure; Ctrl-Q: Quit");
#endif
}

static void load_save_search_json(char *mode) {
  FILE *fp = fopen(SEARCH_SAVE_FILE, mode);
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

static void load_video(char *host, InstanceTypeId instance_type, char *id) {
  int url_len = 0;
  char *n_host;
  char *video_url = NULL;
  char *captions_url = NULL;
  char *m3u8_ptr = NULL;

  strcpy(tmp_buf, id); /* Make it safe, id points to BUF_8K */
  n_host = strdup(host); /* Same for host */

  load_indicator(1);

  /* Build video details URL */
  sprintf((char *)BUF_1K_ADDR,
          video_provider_get_protocol_string(instance_type, VIDEO_DETAILS_ENDPOINT),
          n_host, tmp_buf);
  surl_start_request(NULL, 0, (char *)BUF_1K_ADDR, SURL_METHOD_GET);

  if (!surl_response_ok()) {
    clrscr();
    printf("Error %d", surl_response_code());
    cgetc();
    goto out;
  }

  /* Prefix result with instance URL in case it has no http[s]://host,
   * and video URI into the very large buffer, because Youtube's videos
   * URIs are enormous */
  url_len = strlen(n_host);
  strcpy((char *)BUF_8K_ADDR, n_host);

  if (surl_get_json((char *)(BUF_8K_ADDR + url_len), 
                    video_provider_get_protocol_string(instance_type, VIDEO_URL_JSON_SELECTOR),
                    translit_charset,
                    SURL_HTMLSTRIP_NONE,
                    BUF_8K_SIZE - url_len) > 0) {

    /* If we had an absolute URL without host */
    if (((char *)BUF_8K_ADDR)[url_len] == '/') {
      /* Pass our built URL */
      video_url = (char *)BUF_8K_ADDR;
    } else {
      /* Pass what the server returned */
      video_url = (char *)(BUF_8K_ADDR + url_len);
    }

    if (enable_subtitles) {
      /* Build captions URL */
      sprintf((char *)BUF_1K_ADDR,
              video_provider_get_protocol_string(instance_type, VIDEO_CAPTIONS_ENDPOINT),
              n_host, tmp_buf);

      surl_start_request(NULL, 0, (char *)BUF_1K_ADDR, SURL_METHOD_GET);

      if (surl_response_ok()) {
        char *json_sel = video_provider_get_protocol_string(instance_type, CAPTIONS_JSON_SELECTOR);
        char *lang;
        /* Prefix first result with instance URL */
        strcpy((char *)BUF_1K_ADDR, n_host);

        /* Update preferred subtitle language */
        lang = strstr(json_sel, "LG");
        if (lang) {
          memcpy(lang, sub_language, 2);
        }

        /* Get JSON right after the instance URL */
        if (surl_get_json((char *)(BUF_1K_ADDR + url_len), json_sel,
                          translit_charset, SURL_HTMLSTRIP_NONE,
                          BUF_1K_SIZE - url_len) > 0) {
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
        if (lang) {
          memcpy(lang, "LG", 2);
        }
      }
    }

    load_indicator(0);

    /* Peertube videos streamingPlaylists files */
    if (m3u8_ptr = strstr(video_url, "-fragmented.mp4")) {
      /* Is there an m3u8 playlist? libav* opens them much quicker. */
      strcpy(m3u8_ptr, ".m3u8");
      surl_start_request(NULL, 0, video_url, SURL_METHOD_GET);
      if (!surl_response_ok()) {
        /* No. keep the original file */
        strcpy(m3u8_ptr, "-fragmented.mp4");
      }
    }

    surl_stream_av(captions_url, video_url);
    set_scrollwindow(20, scrh);

#ifdef __APPLE2ENH__
    backup_restore_logo("r");
#endif
    init_hgr(1);
    hgr_mixon();
    clrscr();
  }
out:
  free(n_host);
  load_indicator(0);
}

#pragma code-name(push, "LOWCODE")

char **lines = NULL;
char n_lines;
char cur_line = 0;

static void search_results(InstanceTypeId instance_type) {
  static char *video_host;
  int len;
  char c;

  load_save_search_json("w");
reload_search:
  if (lines) {
    free(lines);
    lines = NULL;
  }
  n_lines = strsplit_in_place((char *)BUF_8K_ADDR, '\n', &lines);
  if (n_lines % N_VIDEO_DETAILS != 0) {
    cputs("Search error              \n");
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

  if (strlen(lines[cur_line+VIDEO_AUTHOR]) > 17)
    lines[cur_line+VIDEO_AUTHOR][17] = '\0';
#endif

  video_host = lines[cur_line+VIDEO_HOST];
  if (video_host[0] == '-') {
    video_host = url;
  }

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
  bzero((char *)HGR_PAGE, HGR_LEN);

  surl_start_request(NULL, 0, lines[cur_line+VIDEO_THUMB], SURL_METHOD_GET);
  if (surl_response_ok()) {
    simple_serial_putc(SURL_CMD_HGR);
    simple_serial_putc(0); /* monochrome */
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
  c = tolower(cgetc());
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
      load_video(video_host, instance_type, lines[cur_line+VIDEO_ID]);
      /* relaunch search */
      load_save_search_json("r");
      goto reload_search;
    case CH_ESC:
      return;
    case CH_CURS_LEFT:
      if (cur_line > N_VIDEO_DETAILS-1) {
        cur_line -= N_VIDEO_DETAILS;
      }
      goto display_result;
    case CH_CURS_RIGHT:
      if (cur_line + N_VIDEO_DETAILS < n_lines) {
        cur_line += N_VIDEO_DETAILS;
      }
      goto display_result;
    default:
      goto read_kbd;
  }
}


static void search(char *host, InstanceTypeId instance_type) {
  sprintf((char *)BUF_1K_ADDR,
          video_provider_get_protocol_string(instance_type, VIDEO_SEARCH_ENDPOINT),
          host, search_str);

  load_indicator(1);

  surl_start_request(NULL, 0, (char *)BUF_1K_ADDR, SURL_METHOD_GET);
  if (!surl_response_ok()) {
    clrscr();
    printf("Error %d", surl_response_code());
    load_indicator(0);
    cgetc();
    return;
  }

  if (surl_get_json((char *)BUF_8K_ADDR,
                    video_provider_get_protocol_string(instance_type, VIDEO_DETAILS_JSON_SELECTOR),
                    translit_charset,
                    SURL_HTMLSTRIP_NONE,
                    BUF_8K_SIZE) > 0) {
    load_indicator(0);
    search_results(instance_type);
  } else {
    clrscr();
    printf("No results.");
    load_indicator(0);
    cgetc();
  }
}

#ifdef __CC65__
#pragma code-name (pop)
#endif

InstanceTypeId global_instance_type;

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
  search(url, global_instance_type);
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
  InstanceTypeId i;

  for (i = 0; i < N_INSTANCE_TYPES; i++) {
    sprintf((char *)BUF_1K_ADDR,
            video_provider_get_protocol_string(i, API_CHECK_ENDPOINT),
            url);

    resp = surl_start_request(NULL, 0, (char *)BUF_1K_ADDR, SURL_METHOD_GET);

    if (surl_response_ok() && resp->size > 0) {
      global_instance_type = i;
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
  int url_len;

  do_setup_url("r");

  if (url == NULL)
    url = strdup("https://sepiasearch.org");

again:
  clrscr();
  //printf("Free memory: %zuB\n", _heapmemavail());
  cputs("Please enter a Peertube or Invidious server URL\r\n");
  cputs("Instance URL: ");
  strcpy((char *)BUF_1K_ADDR, url);
  dget_text((char *)BUF_1K_ADDR, 80, NULL, 0);
  modified = (strcmp((char *)BUF_1K_ADDR, url) != 0);
  free(url);
  url = strdup((char *)BUF_1K_ADDR);

  url_len = strlen(url) - 1;
  if (url[url_len] == '/') {
    url[url_len] = '\0';
  }
  if (url[0] == '\0' || define_instance() < 0) {
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
  surl_user_agent = "WozTubes "VERSION"/Apple II";

#ifdef __APPLE2__
  init_hgr(1);
  hgr_mixon();
  set_scrollwindow(20, scrh);
  clrscr();
#endif

  load_config();

  do_setup();
  printf("started; %zuB free\n", _heapmaxavail());

  do_ui();
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
