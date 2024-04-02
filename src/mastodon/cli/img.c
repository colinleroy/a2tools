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

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "malloc0.h"
#include "extended_conio.h"
#include "clrzone.h"
#include "file_select.h"
#include "scrollwindow.h"
#include "hgr.h"
#include "surl.h"
#include "simple_serial.h"
#include "cli.h"
#include "media.h"
#include "common.h"
#include "path_helper.h"
#include "dgets.h"

char *instance_url;
char *oauth_token;
char *type = NULL;
char *id = NULL;
char monochrome = 1;
unsigned char scrw, scrh;

#ifdef __APPLE2ENH__
  #define TEXTMODE VIDEOMODE_80COL
#else
  #define TEXTMODE VIDEOMODE_40COL
#endif
#ifdef __CC65__
  #pragma rodata-name (push, "HGR")
  char *hgr_page;
  #pragma rodata-name (pop)
#endif

#ifndef __CC65__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#ifndef __CC65__
#pragma GCC diagnostic pop
#endif

static void show_help(void) {
  clrzone(0, 17, NUMCOLS-1, 23);
  cputs("Toggle legend: L\r\n"
        "Save image   : S\r\n"
        "Quit viewer  : Esc\r\n"
        "Next image   : Any other key");
#ifdef __APPLE2__
  gotoxy(0, 22);
  cprintf("%zuB free\r\n", _heapmemavail());
#endif
}

static char legend = 0;
static void toggle_legend(char force) {
  legend = !legend || force;

#ifdef __APPLE2__
  if (legend) {
    init_text();
  } else {
    init_hgr(monochrome);
  }
#endif
}

static void set_legend(char *str, unsigned char video, unsigned char idx, unsigned char num_images) {
  set_hscrollwindow(0, NUMCOLS);
  clrscr();
  cprintf("Image %d/%d: \r\n\r\n", idx + 1, num_images);
  if (str && str[0])
    cputs(str);
  else
    cputs("No description provided :-(");

  if (!video)
    show_help();
}

static void stream_msg(char *msg) {
  hgr_mixon();
  clrscr();
  gotoxy(0, 21);
  cputs(msg);
}
static void video_stream(media *m, char idx, char num_images) {
#ifdef DOUBLE_BUFFER
#ifdef __APPLE2ENH__
  videomode(VIDEOMODE_40COL);
#endif
#endif
  toggle_legend(0);
  stream_msg("Play/Pause : Space\r\n"
             "Quit viewer: Esc\r\n"
             "Waiting for proxy...");
  init_hgr(1);
  hgr_mixon();

  surl_start_request(SURL_METHOD_STREAM_VIDEO, m->media_url[idx], NULL, 0);

#ifdef __CC65__
  if (surl_wait_for_stream() != 0 || surl_stream() != 0) {
#ifdef DOUBLE_BUFFER
#ifdef __APPLE2ENH__
    videomode(VIDEOMODE_80COL);
#endif
#endif
    set_legend("\r\n\r\nRequest failed. Press Esc to exit or another key to restart.", 0, idx, num_images);
    toggle_legend(1);
  } else {
#ifdef DOUBLE_BUFFER
#ifdef __APPLE2ENH__
    videomode(VIDEOMODE_80COL);
#endif
#endif
    stream_msg("\r\n\r\nStream done. Press Esc to exit or another key to restart.");
  }
#endif
}

static void img_display(media *m, char idx, char num_images) {
  size_t len;

  if (!strcmp(m->media_type[idx], "video")) {
    video_stream(m, idx, num_images);
    return;
  }
  surl_start_request(SURL_METHOD_GET, m->media_url[idx], NULL, 0);

  if (surl_response_ok()) {
    #ifndef __CC65__
    #undef HGR_PAGE
    char *HGR_PAGE = malloc0(0x2000);
    #endif

    memset((char *)HGR_PAGE, 0x00, HGR_LEN);

    /* Go to legend while we load */
    toggle_legend(1);
    gotoxy(0, 22);
    cputs("Loading image...");

    simple_serial_putc(SURL_CMD_HGR);
    simple_serial_putc(monochrome);
    simple_serial_putc(HGR_SCALE_FULL);

    if (simple_serial_getc() == SURL_ERROR_OK) {

      surl_read_with_barrier((char *)&len, 2);
      len = ntohs(len);

      if (len == HGR_LEN) {
        toggle_legend(0);
        surl_read_with_barrier((char *)HGR_PAGE, HGR_LEN);

        clrzone(0, 22, NUMCOLS-1, 23);
      } else {
        set_legend("Bad response, not an HGR file.", 0, idx, num_images);
        toggle_legend(1);
      }
    } else {
      set_legend("Request error.", 0, idx, num_images);
      toggle_legend(1);
    }
  } else {
    set_legend("Request failed.", 0, idx, num_images);
    toggle_legend(1);
  }
}

static void save_image(void) {
  unsigned char prev_legend = legend;
  char buf[FILENAME_MAX + 1];
  char *dir = NULL;
  unsigned char x;
  toggle_legend(1);
  clrscr();
  cputs("Save to: ");

  
  dir = file_select((x = wherex()), wherey(), scrw - wherex(), wherey() + 10, 1, "Select directory");
  if (dir == NULL) {
    goto out_no_conf;
  }

  strncpy(buf, dir, FILENAME_MAX);
  free(dir);
  gotox(x);
  strcat(buf, "/");
  dget_text(buf, sizeof(buf) - 1, NULL, 0);

#ifdef __APPLE2__
  _filetype = PRODOS_T_BIN;
  _auxtype = HGR_PAGE;

  if (buf[0] != '\0') {
    FILE *fp = fopen(buf, "w");
    if (fp == NULL) {
      cputs("\r\nCan not open file. ");
      goto out;
    }
    if (fwrite((char *)HGR_PAGE, 1, HGR_LEN, fp) < HGR_LEN) {
      cputs("Can not write to file. ");
      goto out;
    }
    fclose(fp);
    cputs("\r\nImage saved. ");
  } else {
    goto out_no_conf;
  }

out:
#endif
  cputs("Press a key to continue.");
  cgetc();
out_no_conf:
  clrscr();

  if (!prev_legend)
    toggle_legend(0);
}

int main(int argc, char **argv) {
  char *params;
  media *m;
  char i, c;

#ifdef __CC65__
  _heapadd ((void *) 0x0803, 0x17FD);
#endif

#ifdef __APPLE2ENH__
  videomode(TEXTMODE);
#endif

  if (argc < 6) {
    cputs("IMG: Missing parameters.\r\n");
    goto err_out;
  }

  instance_url = argv[1];
  oauth_token = argv[2];
  translit_charset = argv[3];
  monochrome = (argv[4][0] == '1');
  type = argv[5];
  id = argv[6];

  gotoxy(0, 2);
  cputs("Loading medias...\r\n\r\n");

  show_help();

  if (type[0] == 's') {
    m = api_get_status_media(id);
  } else {
    m = api_get_account_media(id);
  }
  if (m == NULL) {
    cputs("Could not load media\r\n");
    goto err_out;
  }
  if (m->n_media == 0) {
    cputs("No images.\r\n");
    goto err_out;
  }

  i = 0;
  while (1) {
    set_legend(m->media_alt_text[i], m->media_type[i][0] == 'v', i, m->n_media);
    img_display(m, i, m->n_media);
getc_again:
    c = cgetc();
    switch(tolower(c)) {
      case CH_ESC:
        goto done;
      case 'l':
        toggle_legend(0);
        goto getc_again;
      case 's':
        save_image();
        set_legend(m->media_alt_text[i], m->media_type[i][0] == 'v', i, m->n_media);
        goto getc_again;
        break;
      default:
        break;
    }
    i++;
    i %= m->n_media;
  }

err_out:
  cgetc();
done:
  if (hgr_init_done) {
    init_text();
  }

  params = malloc0(127);
  snprintf(params, 127, "%s %s", instance_url, oauth_token);
  reopen_start_device();
#ifdef __CC65__
  exec("mastocli", params);
  exit(0);
#else
  printf("exec(mastocli %s)\n",params);
  exit(0);
#endif
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
