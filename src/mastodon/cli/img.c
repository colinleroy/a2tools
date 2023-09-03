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
#ifdef __CC65__
#include <conio.h>
#else
#include <arpa/inet.h>
#include "extended_conio.h"
#endif
#include "clrzone.h"
#include "progress_bar.h"
#include "scrollwindow.h"
#include "surl.h"
#include "simple_serial.h"
#include "media.h"
#include "common.h"

extern char *instance_url;
extern char *oauth_token;
char *type = NULL;
char *id = NULL;
char hgr_init_done = 0;
char monochrome = 1;

#ifdef USE_HGR2
  #define HGR_PAGE 0x4000
  #define TEXTMODE VIDEOMODE_40COL
  #define NUMCOLS 40
  #define PROGRESS_STEPS 32
#else
  #define HGR_PAGE 0x2000
  #define TEXTMODE VIDEOMODE_80COL
  #define NUMCOLS 80
  #define PROGRESS_STEPS 64
  #ifdef __CC65__
    #pragma rodata-name (push, "HGR")
    char *hgr_page;
    #pragma rodata-name (pop)
  #endif
#endif

#ifndef __CC65__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

static void init_hgr(void) {
#ifdef __APPLE2ENH__

#ifdef USE_HGR2
  __asm__("lda     #$40");
#else
  __asm__("lda     #$20");
#endif
  /* Set draw page */
  __asm__("sta     $E6"); /* HGRPAGE */

  /* Set graphics mode */
  __asm__("bit     $C000"); /* 80STORE */
  __asm__("bit     $C050"); /* TXTCLR */
  __asm__("bit     $C052"); /* MIXCLR */
  __asm__("bit     $C057"); /* HIRES */

  /* Set view page */
#ifdef USE_HGR2
  __asm__("bit     $C055"); /* HISCR */
#else
  __asm__("bit     $C054"); /* LOWSCR */
#endif
#endif
  hgr_init_done = 1;
}

#ifndef __CC65__
#pragma GCC diagnostic pop
#endif

void init_text(void) {
#ifdef __APPLE2ENH__
  __asm__("bit     $C054"); /* LOWSCR */
  __asm__("bit     $C051"); /* TXTSET */
  __asm__("bit     $C056"); /* LORES */
#endif
  hgr_init_done = 0;
}

static void print_free_ram(void) {
#ifdef __APPLE2ENH__
  gotoxy(0, 20);
  printf("%zuB free\n", _heapmemavail());
#endif
}

static char legend = 0;
static void toggle_legend(char force) {
  legend = !legend || force;

#ifdef __APPLE2ENH__
  if (legend) {
    init_text();
  } else {
    init_hgr();
  }
#endif
}

static void set_legend(char *str, int idx, int num_images) {
  set_hscrollwindow(0, NUMCOLS);
  clrscr();
  cprintf("Image %d/%d: \r\n\r\n", idx + 1, num_images);
  if (str && str[0])
    cputs(str);
  else
    cputs("No description provided:-(");

  print_free_ram();
}

static void img_display(media *m, char idx, char num_images) {
  size_t len;


  surl_start_request(SURL_METHOD_GET, m->media_url[idx], NULL, 0);

  if (surl_response_ok()) {
    #ifndef __CC65__
    #undef HGR_PAGE
    char *HGR_PAGE = malloc(0x2000);
    #endif

    memset((char *)HGR_PAGE, 0x00, HGR_LEN);

    /* Go to legend while we load */
    toggle_legend(1);
    gotoxy(0, 22);
    cputs("Loading image...");

    simple_serial_putc(SURL_CMD_HGR);
    simple_serial_putc(monochrome);
    if (simple_serial_getc() == SURL_ERROR_OK) {
      simple_serial_read((char *)&len, 2);
      len = ntohs(len);

      if (len == HGR_LEN) {
        int r = 0, b = HGR_LEN/PROGRESS_STEPS;
        progress_bar(0, 23, NUMCOLS, 0, HGR_LEN);
        while (len > 0) {
          simple_serial_read((char *)HGR_PAGE + r, b);
          len -= b;
          r+= b;
          progress_bar(-1, -1, NUMCOLS, r, HGR_LEN);
        }
        clrzone(0, 22, NUMCOLS-1, 23);
        toggle_legend(0);
      } else {
        set_legend("Bad response, not an HGR file.", idx, num_images);
        toggle_legend(1);
      }
    } else {
      set_legend("Request error.", idx, num_images);
      toggle_legend(1);
    }
  } else {
    set_legend("Request failed.", idx, num_images);
    toggle_legend(1);
  }
}

int img_main(int argc, char **argv) {
  char *params;
  media *m;
  char i, c;

  videomode(TEXTMODE);

  if (argc < 6) {
    cputs("Missing parameters.\r\n");
  }

  instance_url = argv[1];
  oauth_token = argv[2];
  translit_charset = argv[3];
  monochrome = (argv[4][0] == '1');
  type = argv[5];
  id = argv[6];

  cputs("\r\n"
        "\r\n"
        "Toggle legend: L\r\n"
        "Quit viewer  : Esc\r\n"
        "Next image   : Any other key\r\n"
        "\r\n"
        "Loading medias...\r\n\r\n");

  print_free_ram();

  if (type[0] == 's') {
    m = api_get_status_media(id);
  } else {
    m = api_get_account_media(id);
  }
  if (m == NULL) {
    cputs("Could not load media\r\n");
    cgetc();
    goto err_out;
  }
  if (m->n_media == 0) {
    cputs("No images.\r\n");
  }

  i = 0;
  while (1) {
    set_legend(m->media_alt_text[i], i, m->n_media);
    img_display(m, i, m->n_media);
getc_again:
    c = cgetc();
    switch(tolower(c)) {
      case CH_ESC:
        goto done;
        break;
      case 'l':
        toggle_legend(0);
        goto getc_again;
        break;
      default:
        break;
    }
    i++;
    i %= m->n_media;
  }

done:
  if (hgr_init_done) {
    init_text();
  }

err_out:
  params = malloc(127);
  snprintf(params, 127, "%s %s", instance_url, oauth_token);
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
