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
#include <ctype.h>
#ifdef __CC65__
#include <conio.h>
#else
#include "extended_conio.h"
#endif
#include "dputs.h"
#include "tgi_compat.h"
#include "surl.h"
#include "simple_serial.h"
#include "api.h"
#include "common.h"

#ifdef __CC65__
/* Init HGR segment */
#pragma rodata-name (push, "HGR")
char *hgr_page1 = (char *)0x2000;
#pragma rodata-name (pop)
#else
char hgr_page1[0x2000] = { 0 };
#endif

char *instance_url = NULL;
char *oauth_token = NULL;
char *status_id = NULL;
char tgi_init_done = 0;

static char mix = 0;
static void toggle_mix(char force, char *str) {
  if (!tgi_init_done)
    return;
  mix = !mix || force;
  
  tgi_apple2_mix(mix);
  gotoxy(0, 0);
  clrscr();
  dputs(str);
}

static void img_display(status_media *s, char idx) {
  surl_response *resp;
  size_t len;

  resp = surl_start_request("GET", s->media_url[idx], NULL, 0);

  if (resp && resp->code >=200 && resp->code < 300) {
    if (!tgi_init_done) {
      tgi_install(a2e_hi_tgi);
      tgi_init();
      tgi_init_done = 1;
    }
    simple_serial_puts("HGR \n");
    if (simple_serial_gets(gen_buf, BUF_SIZE)) {
      len = atoi(gen_buf);
    } else {
      toggle_mix(1, "Could not read response length.");
    }

    if (len == 8192) {
      simple_serial_read(hgr_page1, 1, len);
    } else {
      toggle_mix(1, "Bad response, not an HGR file.");
    }
  } else {
    toggle_mix(1, "Request failed.");
  }
}

int main(int argc, char **argv) {
  char *params;
  status_media *s;
  char i, c;

  videomode(VIDEOMODE_80COL);

  if (argc < 5) {
    printf("Missing instance_url, oauth_token, translit_charset and/or status_id parameters.\n");
  }

  instance_url = argv[1];
  oauth_token = argv[2];
  translit_charset = argv[3];
  status_id = argv[4];

  printf("\n");
  printf("\n");
  printf("Toggle legend: L\n");
  printf("Quit viewer  : Esc\n");
  printf("Next image   : Any other key\n");
  printf("\n");
  printf("Loading...\n");
  s = api_get_status_media(status_id);
  if (s == NULL) {
    printf("Could not load status %s media\n", status_id);
    cgetc();
    goto err_out;
  }
  
  i = 0;
  while (1) {
    img_display(s, i);
    toggle_mix(mix, s->media_alt_text[i]);
getc_again:
    c = cgetc();
    switch(tolower(c)) {
      case CH_ESC:
        goto done;
        break;
      case 'l':
        toggle_mix(0, s->media_alt_text[i]);
        goto getc_again;
        break;
      default:
        break;
    }
    i++;
    i %= s->n_media;
  }

done:
  if (tgi_init_done) {
    tgi_done();
  }

err_out:
  params = malloc(127);
  snprintf(params, 127, "%s %s %s", instance_url, oauth_token, translit_charset);
#ifdef __CC65__
  exec("mastocli", params);
  exit(0);
#else
  printf("exec(mastocli %s)\n",params);
  exit(0);
#endif
}
