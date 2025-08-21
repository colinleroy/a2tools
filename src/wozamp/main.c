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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
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
#include "a2_features.h"

char *data = (char *)splash_hgr;
char *nat_data = NULL;
unsigned int max_nat_data_size = 0;
unsigned char nat_data_static = 0;

extern void AUDIO_CODE_START;
extern unsigned int AUDIO_CODE_SIZE;

char *url_passer_file;

char **lines = NULL;
char **nat_lines = NULL;

#define CENTERX_40COLS 12
#define CENTERX_80COLS 30

unsigned char center_x;
unsigned char NUMCOLS;

extern char **display_lines;

static char in_list = 0;

static int open_url(char *url, char *filename);

#ifdef __CC65__
#pragma code-name (push, "LC")
#endif

static void clr_footer(void) {
  clrzone(0, 22, NUMCOLS - 1, 23);
}

static unsigned char got_cover = 0;
static void display_image(HGRScale scale) {
  size_t len;
  uint8 is_dhgr = 0;

  simple_serial_putc(has_128k ? SURL_CMD_DHGR:SURL_CMD_HGR);
  simple_serial_putc(monochrome);
  simple_serial_putc(scale);
  if (simple_serial_getc() == SURL_ERROR_OK) {

    surl_read_with_barrier((char *)&len, 2);
    len = ntohs(len);

#ifdef __APPLE2__
    is_dhgr = surl_read_image_to_screen(len);
    got_cover = 1;
    init_graphics(monochrome, is_dhgr);
    if (scale == HGR_SCALE_MIXHGR) {
      hgr_mixon();
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
  if (has_80cols) {
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
  } else {
  //      "U,J,Enter,Esc:nav;   /:search;  C:config"
  //      "A:all; R:rnd; S:srv; N:next;    Q: quit"
    cputs("U,J,Enter,Esc:nav;   /:search;  C:config"
          "A:all; R:rnd; S:srv; ");
    if (search_buf[0]) {
      cputs(" N:next;");
    }
    gotox(32);
    cputs("Q: quit");
  }
}

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

char do_radio_browser = 0;

extern char cmd_cb_handled;

static char cmd_cb(char c) {
  char prev_cursor = cursor(0);
  switch (tolower(c)) {
    case 'r':
      do_radio_browser = 1;
      cmd_cb_handled = 1;
      return 1;
    case 'c':
      text_config();
      break;
  }
  cursor(prev_cursor);
  return 0;
}

static void print_err(const char *file) {
  init_text();
  clrscr();
  cputs("Error on ");
  cputs(file);
  cputs(": ");
  citoa(errno);
}

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

#ifdef __CC65__
#pragma code-name (pop)
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
      if (!in_list && enable_video) {
        has_video = 1;
        goto do_video;
      }
      /* Fallthrough to audio */
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
    if (has_80cols) {
      backup_restore_hgrpage("r");
      init_graphics(monochrome, 0);
      hgr_mixon();
    } else {
      init_text();
    }
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
      simple_serial_putc('H');
      simple_serial_putc(SURL_CMD_SKIP);
    } else {
#ifdef __APPLE2__
      simple_serial_putc('D');
      surl_read_image_to_screen(HGR_LEN*2);
      init_graphics(monochrome, 1);
      hgr_mixon();
#endif
      simple_serial_putc(SURL_CLIENT_READY);
    }
    goto read_metadata_again;

  } else if (r == SURL_ANSWER_STREAM_START) {
do_video:
    if (has_video && !in_list && enable_video) {
      FILE *video_url_fp = fopen(url_passer_file, "w");
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
#ifdef __APPLE2__
    backup_restore_audiocode("r");
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

extern char stp_list_scroll_after_url;

#define MOUSETEXT_OPEN_APPLE "\301"
#define FULL_LEN_OPEN_APPLE  "Open-Apple"
#define SHORT_LEN_OPEN_APPLE "App"
#define CONTROL              "Ctrl"
#define FULL_LEN_PROMPT      "Please enter an FTP server or internet stream URL.\r\n"
#define SHORT_LEN_PROMPT     "FTP server or internet stream URL:\r\n"

char *start_url_ui(void) {
  char *url = NULL;
  unsigned char x = 40 - 39;
  char *control_key = CONTROL, *prompt = SHORT_LEN_PROMPT;

start_again:
  clrscr();
  init_graphics(monochrome, 0);
  hgr_mixon();
  set_scrollwindow(20, NUMROWS);

  stp_list_scroll_after_url = 1;

  cmd_cb_handled = 0;
  do_radio_browser = 0;
  if (has_80cols) {
    prompt = FULL_LEN_PROMPT;
    if (is_iieenh) {
      x = 80-16-17;
      control_key = MOUSETEXT_OPEN_APPLE;
    } else { /* consider is_iie true as II+ has no 80 column card */
      x = 80-51;
      control_key = FULL_LEN_OPEN_APPLE;
    }
  } else {
    prompt = SHORT_LEN_PROMPT;
    if (is_iieenh) {
      x = 40-33;
      control_key = MOUSETEXT_OPEN_APPLE;
    } else if (is_iie) {
      x = 40-37;
      control_key = SHORT_LEN_OPEN_APPLE;
    } /* else, see initial values */
  }
  gotoxy(x, 3);
  cputs(control_key);
  cputs("-C: Configure, ");
  cputs(control_key);
  cputs("-R: RadioBrowser");

  gotoxy(0, 0);
  url = stp_get_start_url(prompt,
                        "http://stream.radioparadise.com/rock-320",
                        cmd_cb);

  if (do_radio_browser) {
    search_buf[0] = '\0';
    radio_browser_ui();
    goto start_again;
  }

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
      case CH_CURS_UP:
      case CH_CURS_UP|0x80:
      case 'u':
      case 'u'|0x80:
        while (l--)
          full_update |= stp_list_scroll(-1);
        goto update_list;
      case CH_CURS_DOWN:
      case CH_CURS_DOWN|0x80:
      case 'j':
      case 'j'|0x80:
        while (l--)
          full_update |= stp_list_scroll(+1);
        goto update_list;
      case '/':
        stp_list_search(1);
        stp_print_header(NULL, URL_RESTORE);
        goto keyb_input;
      case 'c'|0x80:
      case 'c':
      case 'C'-'A'+1:
        config();
        break;
      case 'n':
        stp_list_search(0);
        goto keyb_input;
      case 's'|0x80:
      case 's':
      case 'S'-'A'+1:
restart_ui:
        free(url);
        stp_free_data();
        backup_restore_hgrpage("r");
        url = start_url_ui();
        break;
      case 'q'|0x80:
      case 'q':
      case 'Q'-'A'+1:
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
  tmpfp = fopen(url_passer_file, "r");
  if (IS_NULL(tmpfp)) {
    url = start_url_ui();
  } else {
    char *lf;
    url = malloc(512);
    fgets(url, 511, tmpfp);  // URL
    if (IS_NOT_NULL(lf = strchr(url, '\n')))
      *lf = '\0';

    fclose(tmpfp);
    unlink(url_passer_file);
    reopen_start_device();
    set_scrollwindow(0, NUMROWS);
    clrscr();
    init_text();
  }

  do_nav(url);
}

static void init_features(void) {
    if (has_80cols) {
      center_x = CENTERX_80COLS;
      NUMCOLS = 80;
    } else {
      center_x = CENTERX_40COLS;
      NUMCOLS = 40;
    }
    if (has_128k) {
      url_passer_file = RAM_URL_PASSER_FILE;
    } else {
      url_passer_file = URL_PASSER_FILE;
    }
}

void main(void) {
  char *url = NULL;

  /* init bufs */
  search_buf[0] = '\0';
  tmp_buf[0] = '\0';

  try_videomode(VIDEOMODE_80COL);

  init_features();
  serial_throbber_set((void *)0x07F7);

  clrscr();
  init_graphics(monochrome, 0);
  hgr_mixon();
  set_scrollwindow(20, NUMROWS);

  register_start_device();

  reserve_auxhgr_file();

  backup_restore_hgrpage("w");
  if (backup_restore_audiocode("w") == 0) {
    nat_data = &AUDIO_CODE_START;
    max_nat_data_size = (unsigned int)&AUDIO_CODE_SIZE;
    nat_data_static = 1;
  }
  reopen_start_device();

  surl_ping();
  surl_user_agent = "Wozamp "VERSION"/Apple II";

  load_config();

  do_setup();
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
