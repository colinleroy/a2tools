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
#include "extended_conio.h"
#include "dgets.h"
#include "hgr.h"
#include "dputc.h"
#include "clrzone.h"
#include "scroll.h"
#include "scrollwindow.h"
#include "stp_list.h"
#include "runtime_once_clean.h"
#ifdef __APPLE2__
#include "hgr.h"
#endif
#include "path_helper.h"
#include "platform.h"
#include "splash.h"
#include "citoa.h"
#include "backup_logo.h"
#include "radio-browser.h"

char *data = (char *)splash_hgr;
char *nat_data = NULL;

char **lines = NULL;
char **nat_lines = NULL;

#ifdef __APPLE2ENH__
char center_x = 30;
#else
char center_x = 14;
#endif

extern char search_buf[80];
extern char **display_lines;

static char in_list = 0;

static void clr_footer(void) {
  clrzone(0, 22, NUMCOLS - 1, 23);
}

static unsigned char got_cover = 0;
void get_cover_file(char *url) {
  char *cover_url;
  size_t len;

  got_cover = 0;

  len = strlen(url) + 11; /* strlen("/cover.jpg") + 1 */
  cover_url = malloc(len);
  if (!cover_url) {
    /* No memory, well, too bad */
    return;
  }
  if (strchr(url, '/')) {
    strcpy(cover_url, url);
    *strrchr(cover_url, '/') = 0;
    strcat(cover_url, "/cover.jpg");

    surl_start_request(SURL_METHOD_GET, cover_url, NULL, 0);
    if (surl_response_ok()) {
      simple_serial_putc(SURL_CMD_HGR);
      simple_serial_putc(monochrome);
      simple_serial_putc(HGR_SCALE_MIXHGR);
      if (simple_serial_getc() == SURL_ERROR_OK) {

        surl_read_with_barrier((char *)&len, 2);
        len = ntohs(len);

#ifdef __APPLE2__
        if (len == HGR_LEN) {
          surl_read_with_barrier((char *)HGR_PAGE, HGR_LEN);
          got_cover = 1;
        }
#endif
      }
    }
  }
  free(cover_url);
}

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

void stp_print_footer(void) {
  clr_footer();

  gotoxy(0, 22);
#ifdef __APPLE2ENH__
  cputs("Up,Down,Enter,Esc: Navigate     /: Search   C: Configure\r\n"
        "A:Play all files in directory   ");
//      "A:Play all files in directory   N: Next   .-S: Server   .-Q: Quit  (12345B free)");
  if (search_buf[0]) {
    cputs("N: Next");
  }
  gotox(42);
  cputc('A'|0x80);
  cputs("-S: Server   ");
  cputc('A'|0x80);
  cputs("-Q: Quit  (");
  cutoa(_heapmemavail());
  cputs("B free)");
#else
  cputs("U,J,Enter,Esc:nav;  /:search;  C:config\r\n"
        "A:play dir;");
//    "A:play dir; N:next; S:server;  Q: quit"
  if (search_buf[0]) {
    cputs(" N:next;");
  }
  gotox(19);
  cputs(" S:server;  Q: quit");
#endif
}

void show_metadata (char *data) {
  char *value = strchr(data, '\n');
  char x, y, max_len;

  if (value == NULL) {
    return;
  }
  value++;

  x = 0;
  y = 20;
  max_len = NUMCOLS - 1;

  switch(data[3]) {
    //case 'l': /* titLe, nothing to do */
    case 'c': /* traCk */
      max_len = 5;
      x = NUMCOLS - 7;
      break;
    case 'i': /* artIst */
      y = 21;
      break;
    case 'u': /* albUm */
      y = 22;
      break;
  }

  clrzone(x, y, NUMCOLS - 1, y);

  if (strlen(value) >= max_len) {
    value[max_len] = '\0';
  }

  cputs(value);
}

void stp_print_result(const surl_response *response) {
  gotoxy(0, 20);
  chline(NUMCOLS);
  clrzone(0, 21, NUMCOLS - 1, 21);

  if (response == NULL) {
    cputs("Unknown request error.");
  } else if (response->code / 100 != 2){
    cputs("Error: Response code ");
    citoa(response->code);
  }
}

#ifdef __APPLE2ENH__
char do_radio_browser = 0;
#endif

extern char cmd_cb_handled;

static char cmd_cb(char c) {
  char prev_cursor = cursor(0);
  if (tolower(c) == 'r') {
#ifdef __APPLE2ENH__
    do_radio_browser = 1;
    cmd_cb_handled = 1;
    return 1;
#else
    clrscr();
    exec("RBROWSER", NULL);
#endif
  }
  cursor(prev_cursor);
  return 0;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif

#ifdef __CC65__
#pragma code-name (push, "LC")
#endif

#ifndef IIGS
static void print_err(const char *file) {
  init_text();
  clrscr();
  cputs("Error on ");
  cputs(file);
  cputs(": ");
  citoa(errno);
}
#endif

static void play_url(char *url, char *filename) {
  char r;
  char has_video = 0;

  /* Try to get /cover.jpg as a fallback for media with no embedded art */
  get_cover_file(url);

  clrzone(0, 12, NUMCOLS - 1, 12);
  clr_footer();
 
  strncpy(tmp_buf, filename ? filename : "Streaming...", NUMCOLS - 1);
  tmp_buf[NUMCOLS] = '\0';
  cputs(tmp_buf);

  //cputs("Spc:pause, Esc:stop, Left/Right:fwd/rew");
  surl_start_request(SURL_METHOD_STREAM_AUDIO, url, NULL, 0);
  simple_serial_write(translit_charset, strlen(translit_charset));
  simple_serial_putc('\n');
  simple_serial_putc(monochrome);
  simple_serial_putc(HGR_SCALE_MIXHGR);

#ifdef __APPLE2ENH__
  if (!got_cover) {
    backup_restore_logo("r");
  }
  init_hgr(1);
  hgr_mixon();
#elif defined(__APPLE2__)
  if (got_cover) {
    init_hgr(1);
    hgr_mixon();
  } else {
    init_text();
  }
#endif

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

    if (!strncmp(metadata, "has_video\n", 10)) {
      char *value = strchr(metadata, '\n')+1;
      has_video = value[0] == '1';
    } else {
      show_metadata(metadata);
    }
    free(metadata);
    simple_serial_putc(SURL_CLIENT_READY);
    goto read_metadata_again;

  } else if (r == SURL_ANSWER_STREAM_ART) {
    if (got_cover) {
      simple_serial_putc(SURL_CMD_SKIP);
    } else {
#ifdef __APPLE2__
      surl_read_with_barrier((char *)HGR_PAGE, HGR_LEN);
      init_hgr(1);
      hgr_mixon();
#endif
      simple_serial_putc(SURL_CLIENT_READY);
    }
    goto read_metadata_again;

  } else if (r == SURL_ANSWER_STREAM_START) {
#ifndef IIGS
    if (has_video && !in_list && enable_video) {
      FILE *video_url_fp = fopen(URL_PASSER_FILE, "w");
      if (video_url_fp == NULL) {
        print_err("VIDURL");
        cgetc();
        goto novid;
      }
      init_text();
      clrscr();
      gotoxy(center_x, 12);
      cputs("Loading video player...");
      fputs(url, video_url_fp);
      fputc('\n', video_url_fp);
      fputc(enable_subtitles, video_url_fp);
      fputc(video_size, video_url_fp);
      fputs(translit_charset, video_url_fp);

      fclose(video_url_fp);
      simple_serial_putc(SURL_METHOD_ABORT);
      reopen_start_device();
      if (exec("VIDEOPLAY", NULL) != 0) {
        print_err("VIDEOPLAY");
      }
      return;
    }
novid:
#endif
#ifdef __APPLE2__
    surl_stream_audio(NUMCOLS, 20, 2, 23);
    init_text();
#endif
    clr_footer();
    stp_print_footer();

  } else {
#ifdef __APPLE2__
#endif
    gotoxy(0, 21);
    cputs("Playback error");
    cgetc();
    init_text();
  }
}

#ifdef __CC65__
#pragma code-name (pop)
#endif

char *play_directory(char *url) {
  const surl_response *resp;
  int dir_index;

  in_list = 1;
  for (dir_index = 0; dir_index < num_lines; dir_index++) {
    int r;
    url = stp_url_enter(url, lines[dir_index]);
    stp_print_header(display_lines[dir_index], URL_ADD);

    r = stp_get_data(url, &resp);

    if (!resp || resp->code / 100 != 2) {
      break;
    }

    if (r == SAVE_DIALOG) {
      /* Play - warning ! trashes data with HGR page */
      play_url(url, display_lines[dir_index]);
      stp_clr_page();
    }

    /* Fetch original list back */
    url = stp_url_up(url);
    stp_get_data(url, &resp);


    if (!resp || resp->code / 100 != 2) {
      break;
    }

    stp_print_header(NULL, URL_UP);

    /* Stop loop if user pressed esc a second time */
    if (kbhit() && cgetc() == CH_ESC) {
      break;
    }
  }
  in_list = 0;
  return url;
}

char *start_url_ui(void) {
  char *url = NULL;

start_again:
  clrscr();
  init_hgr(1);
  hgr_mixon();
  set_scrollwindow(20, NUMROWS);

#ifdef __APPLE2ENH__
  gotoxy(80-17, 3);
  cputc('A'|0x80);
  cputs("-R: RadioBrowser");
  gotoxy(0, 0);
  cmd_cb_handled = 0;
  do_radio_browser = 0;
  url = stp_get_start_url("Please enter an FTP server or internet stream URL.\r\n",
                        "http://relay.radiofreefedi.net/listen/rff/rff.mp3",
                        cmd_cb);
  if (do_radio_browser) {
    search_buf[0] = '\0';
    radio_browser_ui();
    goto start_again;
  }
#else
  gotoxy(40-20, 3);
  cputs("Ctrl-R: RadioBrowser");
  gotoxy(0, 0);
  url = stp_get_start_url("Please enter an FTP or internet stream\r\n",
                        "http://relay.radiofreefedi.net/listen/rff/rff.mp3",
                        cmd_cb);
#endif

  set_scrollwindow(0, NUMROWS);
  clrscr();
  init_text();

  url = stp_build_login_url(url);
  stp_print_header(url, URL_SET);
  return url;
}


char navigated = 0;

static void do_nav(char *base_url) {
  char full_update = 1;
  char c, l;
  char *url;
  const surl_response *resp;
  char *prev_filename = NULL;

  runtime_once_clean();

  url = strdup(base_url);
  free(base_url);
  stp_print_header(url, URL_SET);

  while(1) {
    if (prev_filename) {
      free(prev_filename);
    }
    prev_filename = (cur_line < num_lines ? strdup(display_lines[cur_line]) : NULL);
    switch (stp_get_data(url, &resp)) {
      case KEYBOARD_INPUT:
        goto keyb_input;
      case SAVE_DIALOG:
        /* Play */
        play_url(url, prev_filename);
        stp_clr_page();
        if (navigated)
          goto up_dir;
        else
          goto restart_ui;
      case UPDATE_LIST:
      default:
        break;
    }

    full_update = 1;
update_list:
    stp_update_list(full_update);

keyb_input:
    stp_print_footer();

    while (!kbhit()) {
      stp_animate_list(0);
    }
    c = tolower(cgetc());
    l = (c & 0x80) ? PAGE_HEIGHT : 1;
    full_update = 0;

    switch(c) {
      case CH_ESC:
up_dir:
        url = stp_url_up(url);
        stp_print_header(NULL, URL_UP);

        full_update = 1;
        break;
      case CH_ENTER:
        if (lines) {
          navigated = 1;
          url = stp_url_enter(url, lines[cur_line]);
          stp_print_header(display_lines[cur_line], URL_ADD);
        }
        break;
      case 'a':
        url = play_directory(url);
        break;
#ifdef __APPLE2ENH__
      case CH_CURS_UP:
      case CH_CURS_UP|0x80:
#else
      case 'u':
      case 'u'|0x80:
#endif
        while (l--)
          full_update |= stp_list_scroll(-1);
        goto update_list;
#ifdef __APPLE2ENH__
      case CH_CURS_DOWN:
      case CH_CURS_DOWN|0x80:
#else
      case 'j':
      case 'j'|0x80:
#endif
        while (l--)
          full_update |= stp_list_scroll(+1);
        goto update_list;
      case '/':
        stp_list_search(1);
        stp_print_header(NULL, URL_RESTORE);
        goto keyb_input;
      case 'c':
        config();
        break;
      case 'n':
        stp_list_search(0);
        goto keyb_input;
#ifdef __APPLE2ENH__
      case 's'|0x80:
#else
      case 's':
#endif
restart_ui:
        free(url);
        stp_free_data();
        backup_restore_logo("r");
        url = start_url_ui();
        break;
#ifdef __APPLE2ENH__
      case 'q'|0x80:
#else
      case 'q':
#endif
        exit(0);
      default:
        goto keyb_input;
    }
  }
  /* Unreachable */
  __asm__("brk");
}

#ifdef __CC65__
#pragma code-name (push, "RT_ONCE")
#endif

static void do_setup(void) {
  FILE *tmpfp;
  char *url = NULL;

  /* Are we back from VIDEOPLAY? */
  tmpfp = fopen(URL_PASSER_FILE, "r");
  if (tmpfp == NULL) {
    url = start_url_ui();
  } else {
    char *lf;
    url = malloc(512);
    fgets(url, 511, tmpfp);  // URL
    if ((lf = strchr(url, '\n')) != NULL)
      *lf = '\0';

    fclose(tmpfp);
    unlink(URL_PASSER_FILE);
    reopen_start_device();
    set_scrollwindow(0, NUMROWS);
    clrscr();
    init_text();
  }

  do_nav(url);
}

void main(void) {
  char *url = NULL;

#ifdef __APPLE2ENH__
  videomode(VIDEOMODE_80COL);
#endif
  clrscr();
  init_hgr(1);
  hgr_mixon();
  set_scrollwindow(20, NUMROWS);

  backup_restore_logo("w");
  surl_ping();
  surl_user_agent = "Wozamp "VERSION"/Apple II";

  load_config();

  do_setup();
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
