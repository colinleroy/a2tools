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

void display_post(post_t *post) {
  surl_start_request(NULL, 0, post->image_url, SURL_METHOD_GET);
  if (surl_response_ok()) {
    bzero((char *)HGR_PAGE, HGR_LEN);
  }
  
  simple_serial_putc(has_128k && can_dhgr ? SURL_CMD_DHGR : SURL_CMD_HGR);
  simple_serial_putc(monochrome);
  simple_serial_putc(HGR_SCALE_FULL);

  if (simple_serial_getc() == SURL_ERROR_OK) {
    size_t len;
    unsigned char is_dhgr, c;

    surl_read_with_barrier((char *)&len, 2);
    len = ntohs(len);
    is_dhgr = surl_read_image_to_screen(len);

    init_graphics(monochrome, is_dhgr);
    set_scrollwindow(20, scrh);
    clrscr();
    cputs(post->description);
    cputs("\r\n By ");
    cputs(post->author);
    cputs(" on ");
    cputs(post->date);
    while (c = tolower(cgetc())) {
      if (c == 'l') {
        if (hgr_mix_is_on) {
          hgr_mixoff();
        } else {
          hgr_mixon();
        }
      } else {
        return;
      }
    }
  }
}

int main(void) {
  post_t *post;
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

  if (!api_login()) {
    cputs("Invalid response.\r\n");
  }

  while (1) {
    post = api_get_post(1);
    display_post(post);
    post_free(post);
  }
}
