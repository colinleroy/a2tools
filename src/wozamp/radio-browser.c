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
#include "malloc0.h"
#include "backup_logo.h"
#include "stp_list.h"

#ifdef __CC65__
#pragma code-name (push, "RT_ONCE")
#endif

#define BUFSIZE 255
char tmp_buf[BUFSIZE];

#define SEARCH_BUF_SIZE 128
char search_buf[SEARCH_BUF_SIZE];
#define JSON_BUF_SIZE 4096
char json_buf[JSON_BUF_SIZE];

unsigned char scrh, scrw;

#define RADIO_BROWSER_API       "http://all.api.radio-browser.info"
#define SEARCH_ENDPOINT         "/json/stations/byname/"
#define SEARCH_PARAMETERS       "?limit=10&order=votes&reverse=true&hidebroken=true"
#define SEARCH_RESULT_SELECTOR  ".[]|select(.hls == 0)|(.name, .homepage, .country, .favicon, .url_resolved, .stationuuid)"
#define CLICK_ENDPOINT          "/json/url/"

enum JsonFieldIdx {
  IDX_NAME = 0,
  IDX_HOMEPAGE,
  IDX_COUNTRY,
  IDX_FAVICON,
  IDX_URL_RESOLVED,
  IDX_STATION_UUID,
  IDX_MAX
};

char **lines = NULL;
char n_lines = 0;
int cur_line;

static void print_footer(void) {
#ifdef __APPLE2ENH__
  if (n_lines > 0) {
    gotoxy(80-37, 2);
    cputs("Enter: Stream radio, Esc: Edit search");
  }
  gotoxy(80-29, 3);
  cputc('A'|0x80);
  cputs("-S: Change server, ");
  cputc('A'|0x80);
  cputs("-Q: Quit");
#else
  gotoxy(40-32, 3);
  cputs("Ctrl-S: new Server, Ctrl-Q: Quit");
#endif
}

static void station_click(char *station_uuid) {
    strcpy(tmp_buf, RADIO_BROWSER_API);
    strcat(tmp_buf, CLICK_ENDPOINT);
    strcat(tmp_buf, station_uuid);
    surl_start_request(SURL_METHOD_GET, tmp_buf, NULL, 0);
}

void show_metadata (char *data) {
  char *value = strchr(data, '\n');
  char max_len;
  if (value == NULL) {
    return;
  }
  value++;
  /* clrzone goes gotoxy */
  if (!strncmp(data, "title\n", 6)) {
    max_len = scrw - 6;
    clrzone(0, 0, max_len, 0);
  }
  if (!strncmp(data, "artist\n", 6)) {
    max_len = scrw - 1;
    clrzone(0, 1, max_len, 1);
  }
  if (strlen(value) > max_len)
    value[max_len] = '\0';

  dputs(value);
}


static void play_url(char *url) {
  char r;

  clrscr();

  surl_start_request(SURL_METHOD_STREAM_AUDIO, url, NULL, 0);
  simple_serial_write(translit_charset, strlen(translit_charset));
  simple_serial_putc('\n');
  simple_serial_putc(monochrome);
  simple_serial_putc(HGR_SCALE_MIXHGR);

read_metadata_again:
  r = simple_serial_getc();
  if (r == SURL_ANSWER_STREAM_METADATA) {
    char *metadata;
    size_t len;
    surl_read_with_barrier((char *)&len, 2);
    len = ntohs(len);
    metadata = malloc(len + 1);
    surl_read_with_barrier(metadata, len);
    metadata[len] = '\0';

    show_metadata(metadata);
    free(metadata);
    simple_serial_putc(SURL_CLIENT_READY);
    goto read_metadata_again;

  } else if (r == SURL_ANSWER_STREAM_ART) {
    surl_read_with_barrier((char *)HGR_PAGE, HGR_LEN);
    simple_serial_putc(SURL_CLIENT_READY);
    goto read_metadata_again;

  } else if (r == SURL_ANSWER_STREAM_START) {
    /* Save new start url */
    FILE *tmpfp = fopen(STP_URL_FILE, "w");
    if (tmpfp) {
      fputs(url, tmpfp);
      fputs("\n\n", tmpfp);
      fclose(tmpfp);
    }
    surl_stream_audio(NUMCOLS, 20, 2, 23);

  } else {
    gotoxy(0, 0);
    dputs("Playback error");
    sleep(1);
  }
}

static void load_indicator(char on) {
#ifdef __APPLE2ENH__
  gotoxy(77, 0);
#else
  gotoxy(37, 0);
#endif
  cputs(on ? "...":"   ");
}

static char cmd_cb(char c) {
  char prev_cursor = cursor(0);
  switch (tolower(c)) {
    case 'q':
      exit(0);
    case 's':
      exec("WOZAMP", NULL);
  }
  cursor(prev_cursor);
  return 0;
}

static void show_results(void) {
  int len;
  char c, n_res, total_res;

  n_lines = strsplit_in_place(json_buf, '\n', &lines);
  if (n_lines % IDX_MAX != 0) {
    cputs("Search error\n");
    cgetc();
    return;
  }
  cur_line = 0;

display_result:
  clrscr();
#ifdef __APPLE2ENH__
  if (strlen(lines[cur_line+IDX_NAME]) > 78)
    lines[cur_line+IDX_NAME][78] = '\0';

  if (strlen(lines[cur_line+IDX_COUNTRY]) > 20)
    lines[cur_line+IDX_COUNTRY][20] = '\0';

  if (strlen(lines[cur_line+IDX_HOMEPAGE]) > 55)
    lines[cur_line+IDX_HOMEPAGE][55] = '\0';
#else
  if (strlen(lines[cur_line+IDX_NAME]) > 38)
    lines[cur_line+IDX_NAME][38] = '\0';

  if (strlen(lines[cur_line+IDX_COUNTRY]) > 13)
    lines[cur_line+IDX_COUNTRY][13] = '\0';

  if (strlen(lines[cur_line+IDX_HOMEPAGE]) > 24)
    lines[cur_line+IDX_HOMEPAGE][24] = '\0';
#endif
   n_res = (cur_line/IDX_MAX)+1;
   total_res = n_lines/IDX_MAX;

  clrscr();
  cprintf("%s\r\n"
         "%s - %s\r\n\r\n"
#ifdef __APPLE2ENH__
         "%c %d/%d results %c",
#else
         "%d/%d",
#endif
         lines[cur_line+IDX_NAME],
         lines[cur_line+IDX_COUNTRY],
         lines[cur_line+IDX_HOMEPAGE],
#ifdef __APPLE2ENH__
         n_res == 1 ? ' ':('H'|0x80), /* Mousetext left arrow */
         n_res, total_res,
         n_res == total_res ? ' ':('U'|0x80));
#else
         n_res, total_res);
 #endif

  print_footer();

  init_text();
  bzero((char *)HGR_PAGE, HGR_LEN);

  if (lines[cur_line+IDX_FAVICON][0] != '\0') {
    load_indicator(1);
    surl_start_request(SURL_METHOD_GET, lines[cur_line+IDX_FAVICON], NULL, 0);
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
  } else {
#ifdef __APPLE2ENH__
    backup_restore_logo("r");
#endif
  }

  init_hgr(1);
  hgr_mixon();
read_kbd:
  c = tolower(cgetc());
#ifdef __APPLE2ENH__
  if (c & 0x80) {
    cmd_cb(c & ~0x80);
    goto read_kbd;
  }
#endif
  switch (c) {
#ifndef __APPLE2ENH__
    case 'S' - 'A' + 1:
    case 'Q' - 'A' + 1:
      cmd_cb(c + 'A' - 1);
      goto read_kbd;
#endif
    case CH_ENTER:
      /* Be a good netizen and register click */
      station_click(lines[cur_line+IDX_STATION_UUID]);
      /* Launch streamer */
      play_url(lines[cur_line+IDX_URL_RESOLVED]);
      goto display_result;
    case CH_ESC:
      free(lines);
      lines = NULL;
      n_lines = 0;
      return;
    case CH_CURS_LEFT:
      if (cur_line > IDX_MAX-1) {
        cur_line -= IDX_MAX;
      }
      goto display_result;
    case CH_CURS_RIGHT:
      if (cur_line + IDX_MAX < n_lines) {
        cur_line += IDX_MAX;
      }
      goto display_result;
    default:
      goto read_kbd;
  }
}

static void search_stations(char *search_str) {
  char *w;

  strcpy(json_buf, RADIO_BROWSER_API);
  strcat(json_buf, SEARCH_ENDPOINT);

  /* very basic urlencoder, encodes everything */
  w = json_buf + strlen(json_buf);
  while (*search_str) {
    snprintf(w, 4, "%%%02x", *search_str);
    w+= 3;
    ++search_str;
  }

  strcat(json_buf, SEARCH_PARAMETERS);

  surl_start_request(SURL_METHOD_GET, json_buf, NULL, 0);

  if (!surl_response_ok()) {
    printf("Error %d", surl_response_code());
    goto err_out;
  }

  if (surl_get_json(json_buf, JSON_BUF_SIZE,
                    SURL_HTMLSTRIP_NONE, translit_charset,
                    SEARCH_RESULT_SELECTOR) > 0) {
    show_results();
    return;
  } else {
    printf("No results.");
    goto err_out;
  }

err_out:
  cgetc();
}

void main(void) {
#ifdef __APPLE2ENH__
  videomode(VIDEOMODE_80COL);
  init_hgr(1);
  hgr_mixon();
#endif
  load_config();
  surl_connect_proxy();

  surl_user_agent = "Wozamp for Apple II / "VERSION;

  screensize(&scrw, &scrh);
  scrh = 24;

  set_scrollwindow(20, scrh);
search_again:
  clrscr();
  print_footer();
  gotoxy(0, 0);
  cputs("Station name: ");
  dget_text(search_buf, SEARCH_BUF_SIZE, cmd_cb, 0);
  cputs("\r\n");
  if (search_buf[0]) {
    search_stations(search_buf);
  }
#ifdef __APPLE2ENH__
  backup_restore_logo("r");
#endif
  goto search_again;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
