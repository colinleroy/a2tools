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
#include "runtime_once_clean.h"
#ifdef __APPLE2__
#include "hgr.h"
#endif
#include "path_helper.h"
#include "platform.h"
#include "splash.h"
#include "malloc0.h"
#include "videoplay.h"

char *data = (char *)splash_hgr;

unsigned char scrw = 255, scrh = 255;

extern char tmp_buf[80];

#pragma code-name(push, "LOWCODE")

static void backup_restore_logo(char *op) {
  FILE *fp = fopen("/RAM/LOGO.HGR", op);
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

static void load_video(char *url, char *id) {
  sprintf((char *)BUF_ADDR, "%s/api/v1/videos/%s", url, id);

  surl_start_request(SURL_METHOD_GET, (char *)BUF_ADDR, NULL, 0);

  if (!surl_response_ok()) {
    return;
  }

  if (surl_get_json((char *)BUF_ADDR, BUF_SIZE, SURL_HTMLSTRIP_NONE, translit_charset,
                    ".formatStreams[]|select(.itag==\"18\").url") >= 0) {
    printf("URL: %s\nHit a key to play", (char *)BUF_ADDR);
    cgetc();
    stream_url((char *)BUF_ADDR);
  }
}

#ifdef __CC65__
#pragma code-name (pop)
#pragma code-name (push, "RT_ONCE")
#endif

static char *do_setup(void) {
  char *url = NULL;

  clrscr();
  url = strdup("https://invidious.fdn.fr/");

  set_scrollwindow(0, scrh);
  #ifdef __APPLE2__
  init_text();
  #endif

  return url;
}

int main(void) {
  char *url = NULL;

#ifdef __APPLE2ENH__
  videomode(VIDEOMODE_80COL);
#endif
#ifdef __APPLE2__
  init_hgr(1);
  hgr_mixon();
#endif
#ifdef __APPLE2ENH__
  backup_restore_logo("w");
#endif
  screensize(&scrw, &scrh);
  set_scrollwindow(20, scrh);
  
  surl_ping();
  load_config();

  url = do_setup();
  printf("started; %zuB free\n", _heapmaxavail());
  cputs("Enter video ID: ");
again:
  dget_text(tmp_buf, 80, NULL, 0);
  load_video(url, tmp_buf);
  goto again;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
