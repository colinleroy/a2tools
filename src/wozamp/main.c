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
#include "backup_hgrpage.h"
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

extern char **display_lines;

static char in_list = 0;

static void clr_footer(void) {
  clrzone(0, 22, NUMCOLS - 1, 23);
}

static unsigned char got_cover = 0;
static void display_image(HGRScale scale) {
  size_t len;
  simple_serial_putc(SURL_CMD_HGR);
  simple_serial_putc(monochrome);
  simple_serial_putc(scale);
  if (simple_serial_getc() == SURL_ERROR_OK) {

    surl_read_with_barrier((char *)&len, 2);
    len = ntohs(len);

#ifdef __APPLE2__
    if (len == HGR_LEN) {
      surl_read_with_barrier((char *)HGR_PAGE, HGR_LEN);
      got_cover = 1;
      init_hgr(1);
      if (scale == HGR_SCALE_MIXHGR) {
        hgr_mixon();
      }
    }
#endif
  }
}

void get_cover_file(char *url) {
  char *cover_url;
  size_t len;

  got_cover = 0;

  len = strlen(url) + 11; /* strlen("/cover.jpg") + 1 */
  cover_url = malloc(len);
  if (IS_NULL(cover_url)) {
    /* No memory, well, too bad */
    return;
  }
  if (IS_NOT_NULL(strchr(url, '/'))) {
    strcpy(cover_url, url);
    *strrchr(cover_url, '/') = 0;
    strcat(cover_url, "/cover.jpg");

    surl_start_request(NULL, 0, cover_url, SURL_METHOD_GET);
    if (surl_response_ok()) {
      display_image(HGR_SCALE_MIXHGR);
    }
  }
  free(cover_url);
}

void stp_print_footer(void) {
  clr_footer();

  gotoxy(0, 22);
#ifdef __APPLE2ENH__
//      "Up,Down,Enter,Esc: Navigate   /: Search                 .-C: Configure");
//      "A:Play all    R:Random        N: Next     .-S: Server   .-Q: Quit  (12345B free)");
  cputs("Up,Down,Enter,Esc: Navigate   /: Search                 ");
  cputc('A'|0x80);                                         cputs("-C: Configure\r\n"
        "A:Play all    R:Random");
  if (search_buf[0]) {
    gotox(30);
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
//      "U,J,Enter,Esc:nav;   /:search;  C:config"
//      "A:all; R:rnd; S:srv; N:next;    Q: quit"
  cputs("U,J,Enter,Esc:nav;   /:search;  C:config"
        "A:all; R:rnd; S:srv; ");
  if (search_buf[0]) {
    cputs(" N:next;");
  }
  gotox(32);
  cputs("Q: quit");
#endif
}

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

void show_metadata (char *data) {
  char *value = strchr(data, '\n');
  char x, y, max_len;

  if (IS_NULL(value)) {
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

void stp_print_result() {
  clrzone(0, 21, NUMCOLS - 1, 21);

  if (!surl_response_ok()) {
    cputs("Error: Response code ");
    citoa(surl_response_code());
  }
}

#ifdef __APPLE2ENH__
char do_radio_browser = 0;
#endif

extern char cmd_cb_handled;

#ifdef __CC65__
#pragma code-name (pop)
#endif

static char cmd_cb(char c) {
  char prev_cursor = cursor(0);
  switch (tolower(c)) {
    case 'r':
#ifdef __APPLE2ENH__
      do_radio_browser = 1;
      cmd_cb_handled = 1;
      return 1;
#else
      clrscr();
      exec("RBROWSER", NULL);
#endif
    case 'c':
      text_config();
      break;
  }
  cursor(prev_cursor);
  return 0;
}

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

static int open_url(char *url, char *filename) {
  char r;
  char has_video = 0;
  int cancelled = 0;
  const char *content_type = surl_content_type();

  clr_footer();
  gotoxy(0, 22);

  if (IS_NOT_NULL(content_type)) {
    if (!strncmp(content_type, "image/", 6)) {
      if (in_list) {
        display_image(HGR_SCALE_FULL);
        platform_interruptible_msleep(10000);
        if (kbhit()) {
          cancelled = (cgetc() == CH_ESC);
        }
      } else {
        display_image(HGR_SCALE_MIXHGR);
      }
      goto out;
    } else if (!strncmp(content_type, "video/", 6)) {
#ifndef IIGS
      if (!in_list && enable_video) {
        has_video = 1;
        goto do_video;
      }
#else
      /* Fallthrough to audio */
#endif
    } else if (!strncmp(content_type, "audio/", 6)) {
      /* fallthrough to audio */
    } else if (!strcmp(content_type, "application/octet-stream")) {
      /* fallthrough to audio, in case it's audio but MIME type
       * is unknown */
    } else {
      cputs("Unsupported file type ");
      cputs(content_type);
      goto err_out;
    }
  }

  /* Try to get /cover.jpg as a fallback for media with no embedded art */
  get_cover_file(url);

  clrzone(0, 12, NUMCOLS - 1, 12);
  clr_footer();

  strncpy(tmp_buf, IS_NOT_NULL(filename) ? filename : "Streaming...", NUMCOLS - 1);
  tmp_buf[NUMCOLS] = '\0';
  cputs(tmp_buf);

  if (!got_cover) {
#ifdef __APPLE2ENH__
    backup_restore_hgrpage("r");
    init_hgr(1);
    hgr_mixon();
#elif defined(__APPLE2__)
    init_text();
#endif
  }

  //cputs("Spc:pause, Esc:stop, Left/Right:fwd/rew");
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
do_video:
    if (has_video && !in_list && enable_video) {
      FILE *video_url_fp = fopen(URL_PASSER_FILE, "w");
      if (IS_NULL(video_url_fp)) {
        print_err("VIDURL");
        cgetc();
        goto novid;
      }
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
      return 0;
    }
novid:
#endif
#ifdef __APPLE2__
    cancelled = surl_stream_audio(NUMCOLS, 20, 2, 23);
    init_text();
#endif

  } else {
    gotoxy(0, 21);
    cputs("Playback error");
err_out:
    cputs("\r\n");
    if (in_list) {
      sleep(1);
    }
out:
    if (!in_list) {
      cputs("Press a key to exit");
      cgetc();
    }
    init_text();
  }
  return cancelled;
}

#ifdef __CC65__
#pragma code-name (push, "LC")
#endif


static int is_streamable(void) {
  const char *content_type;
  content_type = surl_content_type();
  return (!strncmp(content_type, "image/", 6) ||
          !strncmp(content_type, "audio/", 6) ||
          !strncmp(content_type, "video/", 6) ||
          /* Fallback for libmagic failures */
          !strcmp(content_type, "application/octet-stream"));
}

static unsigned char rec_level = 0;
static unsigned char cancelled = 0;
static unsigned char rand_init = 0;

typedef enum {
  PLAY_ALL,
  PLAY_RANDOM
} RecursivePlayMode;

static char *play_directory(RecursivePlayMode mode, char *url);

static char *play_directory_at_index(RecursivePlayMode mode, char *url, unsigned int dir_index) {
  unsigned char r;
  char *prev_filename = strdup(display_lines[dir_index]);

  url = stp_url_enter(url, lines[dir_index]);
  stp_print_header(prev_filename, URL_ADD);

  r = stp_get_data(url);

  if (surl_response_ok()) {
    if (r == SAVE_DIALOG && is_streamable()) {
      /* Play - warning ! trashes data with HGR page */
      cancelled = open_url(url, prev_filename);
      stp_clr_page();
    } else if (r == UPDATE_LIST && rec_level < 4) {
      /* That's a directory */
      rec_level++;
      url = play_directory(mode, url);
      rec_level--;
    }
  }

  free(prev_filename);

  /* Fetch original list back */
  url = stp_url_up(url);
  stp_get_data(url);

  if (surl_response_ok()) {
    stp_print_header(NULL, URL_UP);
  } else {
    cancelled = 1;
  }

  return url;
}

static char *play_directory(RecursivePlayMode mode, char *url) {
  unsigned int dir_index;

  cancelled = 0;

  if (mode == PLAY_ALL) {
    for (dir_index = cur_line; dir_index < num_lines; dir_index++) {
      in_list = 1;
      url = play_directory_at_index(PLAY_ALL, url, dir_index);
      if (cancelled) {
        break;
      }
    }
  } else if (mode == PLAY_RANDOM) {
    if (!rand_init) {
      _randomize();
      rand_init = 1;
    }
    while (!cancelled) {
      /*
       * 0          0
       * rand()     dir_index
       * RAND_MAX   num_lines-1
       */
      dir_index = (uint16)((((uint32)rand() * (num_lines-1)))/RAND_MAX);
      /* Set in_list each time (going back up all the way resets it) */
      in_list = 1;
      url = play_directory_at_index(PLAY_RANDOM, url, dir_index);
      if (rec_level > 0) {
        /* go up all the way if we entered subdirectories */
        break;
      }
    }
  }
  in_list = 0;
  return url;
}

extern char stp_list_scroll_after_url;

char *start_url_ui(void) {
  char *url = NULL;

start_again:
  clrscr();
  init_hgr(1);
  hgr_mixon();
  set_scrollwindow(20, NUMROWS);

  stp_list_scroll_after_url = 1;
#ifdef __APPLE2ENH__
  gotoxy(80-16-17, 3);
  cputc('A'|0x80);
  cputs("-C: Configure, ");
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
  gotoxy(40-39, 3);
  cputs("Ctrl-C: Configure, Ctrl-R: RadioBrowser");
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

#ifdef __CC65__
#pragma code-name (pop)
#endif

char navigated = 0;

static void do_nav(char *base_url) {
  char full_update = 1;
  char c, l;
  char *url;
  char *prev_filename = NULL;

  runtime_once_clean();

  url = strdup(base_url);
  free(base_url);
  stp_print_header(url, URL_SET);

  while(1) {
    if (IS_NOT_NULL(prev_filename)) {
      free(prev_filename);
    }
    prev_filename = (cur_line < num_lines ? strdup(display_lines[cur_line]) : NULL);
    switch (stp_get_data(url)) {
      case KEYBOARD_INPUT:
        goto keyb_input;
      case SAVE_DIALOG:
        /* Play */
        if (is_streamable()) {
          open_url(url, prev_filename);
          stp_clr_page();
        } else {
          beep();
        }
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
        if (IS_NOT_NULL(lines)) {
          navigated = 1;
          url = stp_url_enter(url, lines[cur_line]);
          stp_print_header(display_lines[cur_line], URL_ADD);
        }
        break;
      case 'a':
        rec_level = 0;
        url = play_directory(PLAY_ALL, url);
        break;
      case 'r':
        rec_level = 0;
        url = play_directory(PLAY_RANDOM, url);
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
#ifdef __APPLE2ENH__
      case 'c'|0x80:
#else
      case 'c':
#endif
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
        backup_restore_hgrpage("r");
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
  if (IS_NULL(tmpfp)) {
    url = start_url_ui();
  } else {
    char *lf;
    url = malloc(512);
    fgets(url, 511, tmpfp);  // URL
    if (IS_NOT_NULL(lf = strchr(url, '\n')))
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

  /* init bufs */
  search_buf[0] = '\0';
  tmp_buf[0] = '\0';

#ifdef __APPLE2ENH__
  videomode(VIDEOMODE_80COL);
#endif
  serial_throbber_set((void *)0x07F7);

  clrscr();
  init_hgr(1);
  hgr_mixon();
  set_scrollwindow(20, NUMROWS);

  backup_restore_hgrpage("w");
  surl_ping();
  surl_user_agent = "Wozamp "VERSION"/Apple II";

  load_config();

  do_setup();
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
