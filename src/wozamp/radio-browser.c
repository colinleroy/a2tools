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
#include "dget_text.h"
#include "dputc.h"
#include "clrzone.h"
#include "scroll.h"
#include "scrollwindow.h"
#ifdef __APPLE2__
#include "hgr.h"
#endif
#include "path_helper.h"
#include "platform.h"
#include "malloc0.h"
#include "backup_hgrpage.h"
#include "citoa.h"
#include "a2_features.h"

#include "stp_list.h"

char n_lines = 0;

#define JSON_BUF_SIZE 4096
char *json_buf = NULL;

#define RADIO_BROWSER_API       "http://all.api.radio-browser.info"
#define SEARCH_ENDPOINT         "/json/stations/byname/"
#define SEARCH_PARAMETERS       "?limit=10&order=votes&reverse=true&hidebroken=true"
#define SEARCH_RESULT_SELECTOR  ".[]|select(.hls == 0)|(.name, .homepage, .country, .favicon, .url_resolved, .stationuuid)"
#define CLICK_ENDPOINT          "/json/url/"
#define RADIO_SEARCH_FILE       "RBRESULTS"

enum JsonFieldIdx {
  IDX_NAME = 0,
  IDX_HOMEPAGE,
  IDX_COUNTRY,
  IDX_FAVICON,
  IDX_URL_RESOLVED,
  IDX_STATION_UUID,
  IDX_MAX
};

static void print_footer(void) {
  if (has_80cols) {
    if (n_lines > 0) {
      gotoxy(80-37, 2);
      cputs("Enter: Stream radio, Esc: Edit search");
    }
    if (is_iieenh) {
      gotoxy(80-45, 3);
      cputc('A'|0x80);
      cputs("-C: Configure, ");
      cputc('A'|0x80);
      cputs("-S: Change server, ");
      cputc('A'|0x80);
      cputs("-Q: Quit");
    } else {
      gotoxy(80-69, 3);
      cputs("Open-Apple-C: Configure, Open-Apple-S: new Server, Open-Apple-Q: Quit");
    }
  } else {
    gotoxy(40-32, 3);
    if (is_iie) {
      cputs("App-S: new Server, App-Q: Quit");
    } else {
      cputs("Ctrl-S: new Server, Ctrl-Q: Quit");
    }
  }
}

static void station_click(char *station_uuid) {
    strcpy(tmp_buf, RADIO_BROWSER_API);
    strcat(tmp_buf, CLICK_ENDPOINT);
    strcat(tmp_buf, station_uuid);
    surl_start_request(NULL, 0, tmp_buf, SURL_METHOD_GET);
}

void show_radio_metadata (char *data) {
  char *value = strchr(data, '\n');
  char max_len;
  if (IS_NULL(value)) {
    return;
  }
  value++;
  /* clrzone goes gotoxy */
  if (!strncmp(data, "title\n", 6)) {
    max_len = NUMCOLS - 6;
    clrzone(0, 0, max_len, 0);
  }
  if (!strncmp(data, "artist\n", 6)) {
    max_len = NUMCOLS - 1;
    clrzone(0, 1, max_len, 1);
  }
  if (strlen(value) > max_len)
    value[max_len] = '\0';

  cputs(value);
}

static char do_server_screen = 0;
static char cmd_cb(char c) {
  char prev_cursor = cursor(0);
  switch (tolower(c)) {
    case 'q':
      exit(0);
    case 's':
      do_server_screen = 1;
      return 1;
    case 'c':
      text_config();
      break;
  }
  cursor(prev_cursor);
  return 0;
}

static void play_url(char *url) {
  char r;

  clrscr();

  surl_start_request(NULL, 0, url, SURL_METHOD_STREAM_AUDIO);
  simple_serial_puts_nl(translit_charset);
  simple_serial_putc(monochrome);
  simple_serial_putc(HGR_SCALE_MIXHGR);

read_metadata_again:
  r = simple_serial_getc();
  if (r == SURL_ANSWER_STREAM_LOAD) {
    simple_serial_putc(SURL_CLIENT_READY);
    simple_serial_getc(); /* ETA */
    if (kbhit() && cgetc() == CH_ESC)
      simple_serial_putc(SURL_METHOD_ABORT);
    else
      simple_serial_putc(SURL_CLIENT_READY);
    goto read_metadata_again;
  } else if (r == SURL_ANSWER_STREAM_METADATA) {
    char *metadata;
    size_t len;
    surl_read_with_barrier((char *)&len, 2);
    len = ntohs(len);

    metadata = malloc(len + 1);
    surl_read_with_barrier(metadata, len);
    metadata[len] = '\0';
    show_radio_metadata(metadata);
    free(metadata);

    simple_serial_putc(SURL_CLIENT_READY);
    goto read_metadata_again;

  } else if (r == SURL_ANSWER_STREAM_ART) {
    surl_read_with_barrier((char *)HGR_PAGE, HGR_LEN);
    simple_serial_putc(SURL_CLIENT_READY);
    goto read_metadata_again;

  } else if (r == SURL_ANSWER_STREAM_START) {
    /* Save new start url */
    FILE *tmpfp = fopen(RADIO_SEARCH_FILE, "w");
    if (IS_NOT_NULL(tmpfp)) {
      fputc(cur_line, tmpfp);
      fputs(search_buf, tmpfp);
      fclose(tmpfp);
    }
    surl_stream_audio(NUMCOLS, 20, 2, 23);

  } else {
    gotoxy(0, 0);
    cputs("Playback error");
    sleep(1);
  }
}

static void show_results(void) {
  int len;
  char c, n_res, total_res;
  char *tmp;

  n_lines = strsplit_in_place(json_buf, '\n', &lines);
  if (n_lines % IDX_MAX != 0) {
    cputs("Search error\n");
    cgetc();
    return;
  }

  if (cur_line >= n_lines)
    cur_line = 0;

display_result:
  clrscr();

  n_res = (cur_line/IDX_MAX)+1;
  total_res = n_lines/IDX_MAX;

  if (has_80cols) {
    tmp = lines[cur_line+IDX_NAME];
    if (strlen(tmp) > 78)
      tmp[78] = '\0';

    cputs(tmp);
    cputs("\r\n");

    tmp = lines[cur_line+IDX_COUNTRY];
    if (strlen(tmp) > 20)
      tmp[20] = '\0';

    cputs(tmp);
    cputs(" - ");

    tmp = lines[cur_line+IDX_HOMEPAGE];
    if (strlen(tmp) > 55)
      tmp[55] = '\0';

    cputs(tmp);
    cputs("\r\n\r\n");
    cputc(!is_iieenh || n_res == 1 ? ' ':('H'|0x80));
    cutoa(n_res);
    cputc('/');
    cutoa(total_res);
    cputc(!is_iieenh || n_res == total_res ? ' ':('U'|0x80));
  } else {
    tmp = lines[cur_line+IDX_NAME];
    if (strlen(tmp) > 38)
      tmp[38] = '\0';

    cputs(tmp);
    cputs("\r\n");

    tmp = lines[cur_line+IDX_COUNTRY];
    if (strlen(tmp) > 13)
      tmp[13] = '\0';

    cputs(tmp);
    cputs(" - ");

    tmp = lines[cur_line+IDX_HOMEPAGE];
    if (strlen(tmp) > 23)
      tmp[23] = '\0';

    cputs(tmp);
    cputs("\r\n\r\n");
    cutoa(n_res);
    cputc('/');
    cutoa(total_res);
  }

  print_footer();

  init_text();
  bzero((char *)HGR_PAGE, HGR_LEN);

  if (lines[cur_line+IDX_FAVICON][0] != '\0') {
    surl_start_request(NULL, 0, lines[cur_line+IDX_FAVICON], SURL_METHOD_GET);
    if (surl_response_ok()) {
      simple_serial_putc(SURL_CMD_HGR);
      simple_serial_putc(monochrome);
      simple_serial_putc(HGR_SCALE_MIXHGR);

      if (simple_serial_getc() == SURL_ERROR_OK) {

        surl_read_with_barrier((char *)&len, 2);
        len = ntohs(len);

        if (len == HGR_LEN) {
          surl_read_with_barrier((char *)HGR_PAGE, HGR_LEN);
        }
      }
    }
  } else {
    backup_restore_hgrpage("r");
  }

  init_hgr(1);
  hgr_mixon();
read_kbd:
  c = tolower(cgetc());
  if (is_iie && c & 0x80) {
    cmd_cb(c & ~0x80);
check_cmd:
    if (do_server_screen) {
exit_results:
      free(lines);
      lines = NULL;
      n_lines = 0;
      return;
    }
    goto read_kbd;
  } else if (!is_iie && c < 27 && c != CH_ENTER && c != CH_CURS_LEFT && c != CH_CURS_RIGHT) {
    cmd_cb(c + 'A' - 1);
    goto check_cmd;
  }
  switch (c) {
    case CH_ENTER:
      /* Be a good netizen and register click */
      station_click(lines[cur_line+IDX_STATION_UUID]);
      /* Launch streamer */
      play_url(lines[cur_line+IDX_URL_RESOLVED]);
      goto display_result;
    case CH_ESC:
      goto exit_results;
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

#pragma code-name(push, "LC")

static void search_stations(char *search_str) {
  char *w;

  json_buf = malloc0(JSON_BUF_SIZE);

  strcpy(json_buf, RADIO_BROWSER_API);
  strcat(json_buf, SEARCH_ENDPOINT);

  /* very basic urlencoder, encodes everything */
  w = json_buf + strlen(json_buf);
  while (*search_str) {
    *w = '%';
    w++;
    utoa(*search_str, w, 16);
    w+= 2;
    ++search_str;
  }

  strcat(json_buf, SEARCH_PARAMETERS);

  surl_start_request(NULL, 0, json_buf, SURL_METHOD_GET);

  if (!surl_response_ok()) {
    cputs("Error ");
    cutoa(surl_response_code());
    goto err_out;
  }

  if (surl_get_json(json_buf, SEARCH_RESULT_SELECTOR,
                    translit_charset, SURL_HTMLSTRIP_NONE,
                    JSON_BUF_SIZE) > 0) {
    show_results();
    goto out;
  } else {
    cputs("No results.");
    goto err_out;
  }

err_out:
  cgetc();
out:
  free(json_buf);
}

#pragma code-name(pop)

void radio_browser_ui(void) {
  FILE *tmpfp = fopen(RADIO_SEARCH_FILE, "r");

  init_hgr(1);
  hgr_mixon();

  do_server_screen = 0;
  set_scrollwindow(20, NUMROWS);

  if (IS_NOT_NULL(tmpfp)) {
    cur_line = fgetc(tmpfp);
    fgets(search_buf, SEARCH_BUF_SIZE-1, tmpfp);
    fclose(tmpfp);
    search_stations(search_buf);
    if (do_server_screen) {
      goto out;
    }
  }

search_again:
  clrscr();
  print_footer();
  gotoxy(0, 0);
  cputs("Station name: ");
  dget_text_single(search_buf, SEARCH_BUF_SIZE, cmd_cb);
  if (do_server_screen) {
    return;
  }
  cputs("\r\n");
  if (search_buf[0]) {
    cur_line = 0;
    search_stations(search_buf);
  }
  backup_restore_hgrpage("r");
  if (!do_server_screen) {
    goto search_again;
  }
out:
  search_buf[0] = '\0';
}
