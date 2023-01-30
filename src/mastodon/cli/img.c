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
#include "extended_conio.h"
#endif
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
char hgr_init_done = 0;
char monochrome = 1;

static char mix = 0;
static void toggle_mix(char force, char *str) {
  if (!hgr_init_done)
    return;
  mix = !mix || force;

#ifdef __APPLE2ENH__
  if (mix) {
    __asm__("ldx     #1");
    __asm__("lda     #20");
    __asm__("sta     $22"); /* WNDTOP */
    __asm__("bit     $C053"); /* MIXSET */
  } else {
    __asm__("ldx     #0");
    __asm__("lda     #00");
    __asm__("sta     $22"); /* WNDTOP */
    __asm__("bit     $C052"); /* MIXCLR */
  }
#endif

  gotoxy(0, 0);
  clrscr();
  cputs(str);
}

static void img_display(status_media *s, char idx) {
  surl_response *resp;
  size_t len;

  resp = surl_start_request("GET", s->media_url[idx], NULL, 0);

  if (resp && resp->code >=200 && resp->code < 300) {
    if (!hgr_init_done) {
#ifdef __APPLE2ENH__
        __asm__("bit     $C052"); /* MIXCLR */
        __asm__("bit     $C057"); /* HIRES */
        __asm__("sta     $C07E"); /* IOUDISOU */
        __asm__("bit     $C05F"); /* DHIRESOFF */
        __asm__("bit     $C050"); /* TXTCLR */
#endif
      hgr_init_done = 1;
    }
    simple_serial_printf("HGR %d\n", monochrome);
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
  surl_response_free(resp);
}

int main(int argc, char **argv) {
  FILE *fp;
  char *params;
  status_media *s;
  char i, c;

  videomode(VIDEOMODE_80COL);

  if (argc < 5) {
    cputs("Missing instance_url, oauth_token, translit_charset and/or status_id parameters.\r\n");
  }

  instance_url = argv[1];
  oauth_token = argv[2];
  translit_charset = argv[3];
  status_id = argv[4];

  cputs("\r\n");
  cputs("\r\n");
  cputs("Toggle legend: L\r\n");
  cputs("Quit viewer  : Esc\r\n");
  cputs("Next image   : Any other key\r\n");
  cputs("\r\n");
  cputs("Loading...\r\n");

  fp = fopen("clisettings", "r");
  translit_charset = US_CHARSET;

  if (fp != NULL) {
    char buf[16];
    fgets(buf, 16, fp);
    fgets(buf, 16, fp);
    monochrome = atoi(buf);
    fclose(fp);
  }

  s = api_get_status_media(status_id);
  if (s == NULL) {
    cputs("Could not load status media\r\n");
    cgetc();
    goto err_out;
  }
  if (s->n_media == 0) {
    cputs("No images in status.\r\n");
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
  if (hgr_init_done) {
#ifdef __APPLE2ENH__
    __asm__("bit     $C051"); /* TXTSET */
    __asm__("bit     $C054"); /* LOWSCR */
    __asm__("bit     $C056"); /* LORES */
    __asm__("lda     #$00");
    __asm__("sta     $22"); /* WNDTOP */
#endif
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

#ifdef __CC65__
#pragma code-name (pop)
#endif
