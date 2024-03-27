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
#include "dputc.h"
#include "dputs.h"
#include "clrzone.h"
#include "scroll.h"
#include "scrollwindow.h"
#include "stp_list.h"
#include "runtime_once_clean.h"
#include "hgr.h"
#include "pwm.h"
#include "platform.h"
#include "splash.h"

char *data = (char *)splash_hgr;
char *nat_data = NULL;

char **lines = NULL;
char **nat_lines = NULL;

unsigned char scrw = 255, scrh = 255;
char center_x = 14; /* 30 in 80COLS */

extern char search_buf[80];
extern char **display_lines;

void stp_print_footer(void) {

  clrzone(0, 22, scrw - 1, 23);

  gotoxy(0, 22);
  #ifdef __APPLE2ENH__
  dputs("Up,Down,Enter,Esc:nav /:search C:config");
  #else
  dputs("U,J,Enter,Esc:nav /:search C:config");
  #endif

  dputs("\r\nA:play all files in directory");
  if (search_buf[0]) {
    dputs(" N:next");
  }
}

void stp_print_result(const surl_response *response) {
  gotoxy(0, 20);
  chline(scrw);
  clrzone(0, 21, scrw - 1, 21);

  if (response == NULL) {
    dputs("Unknown request error.");
  } else if (response->code / 100 != 2){
    cprintf("Error: Response code %d - %lu bytes",
            response->code,
            response->size);
  }
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
    clrzone(0, 20, max_len, 20);
  }
  if (!strncmp(data, "track\n", 6)) {
    max_len = 5;
    clrzone(scrw - 6, 20, scrw - 6 + max_len, 20);
  }
  if (!strncmp(data, "artist\n", 6)) {
    max_len = scrw - 1;
    clrzone(0, 21, max_len, 21);
  }
  if (!strncmp(data, "album\n", 6)) {
    max_len = scrw - 1;
    clrzone(0, 22, max_len, 22);
  }
  if (strlen(value) > max_len)
    value[max_len] = '\0';

  dputs(value);
}

#ifdef __CC65__
#pragma code-name (push, "LC")
#endif


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

        if (len == HGR_LEN) {
          surl_read_with_barrier((char *)HGR_PAGE, HGR_LEN);
          got_cover = 1;
        }
      }
    }
  }
  free(cover_url);
}

#ifdef __APPLE2ENH__
static void backup_restore_logo(char *op) {
  FILE *fp = fopen("/RAM/WOZAMP.HGR", op);
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

extern char tmp_buf[80];
static void play_url(char *url, char *filename) {
  char r;

  /* Try to get /cover.jpg as a fallback for media with no embedded art */
  get_cover_file(url);

  clrzone(0, 12, scrw - 1, 12);
  clrzone(0, 20, scrw - 1, 23);
 
  strncpy(tmp_buf, filename ? filename : "Streaming...", scrw - 1);
  tmp_buf[scrw] = '\0';
  gotoxy(0, 20);
  dputs(tmp_buf);
  //dputs("Spc:pause, Esc:stop, Left/Right:fwd/rew");
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
#else
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
    show_metadata(metadata);
    free(metadata);
    simple_serial_putc(SURL_CLIENT_READY);
    goto read_metadata_again;

  } else if (r == SURL_ANSWER_STREAM_ART) {
    if (got_cover) {
      simple_serial_putc(SURL_CMD_SKIP);
    } else {
      surl_read_with_barrier((char *)HGR_PAGE, HGR_LEN);
      init_hgr(1);
      hgr_mixon();
      simple_serial_putc(SURL_CLIENT_READY);
    }
    goto read_metadata_again;

  } else if (r == SURL_ANSWER_STREAM_START) {
    simple_serial_putc(SURL_CLIENT_READY);
    pwm(2, 23);
    init_text();
    clrzone(0, 20, scrw - 1, 23);
    stp_print_footer();

  } else {
    init_text();
    gotoxy(center_x, 10);
    dputs("Playback error");
    sleep(1);
  }
}

char *play_directory(char *url) {
  const surl_response *resp;
  int dir_index;

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
      clrzone(0, PAGE_BEGIN, scrw - 1, PAGE_BEGIN + PAGE_HEIGHT);
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
  return url;
}

char navigated = 0;

int main(void) {
  char *url = NULL;
  char full_update = 1;
  char c;
  const surl_response *resp;

  init_hgr(1);
  hgr_mixon();
#ifdef __APPLE2ENH__
  backup_restore_logo("w");
#endif
  screensize(&scrw, &scrh);
  set_scrollwindow(20, scrh);
  
  surl_ping();
#ifdef __CC65__
  printf("Mem available: %zub\n", _heapmaxavail());
#endif
  runtime_once_clean();

  load_config();

restart:
  if (url) {
    free(url);
    url = NULL;
  }

  clrscr();
  url = stp_get_start_url("Please enter an FTP or internet stream\r\n",
                          "http://8bit.fm:8000/live");
  url = stp_build_login_url(url);

  set_scrollwindow(0, scrh);
  init_text();
  stp_print_header(url, URL_SET);

  stp_print_footer();

  while(1) {
    char *prev_filename = (cur_line < num_lines ?
                            strdup(display_lines[cur_line]) : NULL);
    switch (stp_get_data(url, &resp)) {
      case KEYBOARD_INPUT:
        goto keyb_input;
      case SAVE_DIALOG:
        /* Play */
        play_url(url, prev_filename);
        clrzone(0, PAGE_BEGIN, scrw - 1, PAGE_BEGIN + PAGE_HEIGHT);
        if (navigated)
          goto up_dir;
        else
          goto restart;
      case UPDATE_LIST:
      default:
        break;
    }
    if (prev_filename) {
      free(prev_filename);
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
#else
      case 'u':
#endif
        full_update = stp_list_scroll(-1);
        goto update_list;
#ifdef __APPLE2ENH__
      case CH_CURS_DOWN:
#else
      case 'j':
#endif
        full_update = stp_list_scroll(+1);
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
      default:
        goto update_list;
    }
  }

  exit(0);
}
#ifdef __CC65__
#pragma code-name (pop)
#endif
