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
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "extended_conio.h"
#include "dget_text.h"
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
#include "splash.h"
#include "citoa.h"
#include "backup_hgrpage.h"
#include "a2_features.h"
#include "api.h"
#include "surl.h"

unsigned char scrw = 255, scrh = 255;
unsigned char monochrome = 1;

#define BUF_SIZE 511
char gen_buf[BUF_SIZE+1];

void cleanup(void) {
}

static unsigned char dhgr_init_done = 0;
static char last_displayed[16] = "";

void display_post(post_t *post) {
  /* If re-displaying same post, spare the query. */
  if (!strcmp(post->id, last_displayed)) {
    goto print_description;
  }

  surl_start_request(NULL, 0, post->image_url, SURL_METHOD_GET);

  if (!surl_response_ok()) {
    return;
  }
  
  simple_serial_putc(has_128k && can_dhgr ? SURL_CMD_DHGR : SURL_CMD_HGR);
  simple_serial_putc(monochrome);
  simple_serial_putc(HGR_SCALE_FULL);

  if (simple_serial_getc() == SURL_ERROR_OK) {
    size_t len;
    unsigned char is_dhgr;

    surl_read_with_barrier((char *)&len, 2);
    len = ntohs(len);
    is_dhgr = surl_read_image_to_screen(len);

    if (!dhgr_init_done) {
      init_graphics(monochrome, is_dhgr);
      hgr_mixon(); /* Legend on by default */
      dhgr_init_done = 1;
      set_scrollwindow(20, scrh);
    }
    strcpy(last_displayed, post->id);

print_description:
    clrscr();
    cputs(post->description);
    gotoxy(0, 3);
    cputs(" By ");
    cputs(post->author);
    cputs(" on ");
    cputs(post->date);
  }
}

static void show_help(void) {
  clrscr();
  cputs("Left: previous post; Next: next post; L: toggle legend; M: toggle color\r\n"
        "Press a key to return...");
  cgetc();
}

int main(void) {
  post_t *post = NULL;
  unsigned char shift = 1, c;

  register_start_device();

  reserve_auxhgr_file();  /* Sets can_dhgr */

  atexit(&cleanup);

  try_videomode(VIDEOMODE_80COL);

  serial_throbber_set((void *)0x07F7);

  screensize(&scrw, &scrh);
  surl_ping();
  surl_user_agent = "SixForty "VERSION"/Apple II";

#ifdef __APPLE2__
  init_graphics(monochrome, 0);
  hgr_mixon();
  set_scrollwindow(20, scrh);
  clrscr();
#endif

  cprintf("Welcome to SixForty. %zuB free\r\n", _heapmaxavail());

  while (api_login() != 0) {
    clrscr();
    cputs("Login failed.\r\n");
  }

  while (1) {
    post_free(post);
    post = api_get_post(shift);
    if (IS_NULL(post)) {
      clrscr();
      cputs("Could not load post :-/\r\n");
    } else {
display_again:
      display_post(post);
    }
get_command:
    c = tolower(cgetc());
    switch (c) {
      case CH_CURS_LEFT:      /* previous */
        shift = -1;
        break;
      case CH_CURS_RIGHT:     /* next */
        shift = 1;
        break;
      case 'h':
        show_help();
        goto display_again;
      case 'm':
        monochrome = !monochrome;
        dhgr_init_done = 0;
        last_displayed[0] = '\0'; /* Force reload to re-convert */
        goto display_again;
      case 'l':               /* legend */
        if (hgr_mix_is_on) {
          hgr_mixoff();
        } else {
          hgr_mixon();
        }
        /* Fallthrough to next command */
      default:
        goto get_command;
    }
  }
}
