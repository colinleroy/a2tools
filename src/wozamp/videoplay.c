#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <conio.h>
#include <unistd.h>
#include "hgr.h"
#include "simple_serial.h"
#include "path_helper.h"
#include "surl.h"
#include "config.h"
#include "splash-video.h"
#include "scrollwindow.h"
#include "surl/surl_stream_av/stream_url.h"

char *translit_charset;
char monochrome = 1;
unsigned char scrw = 255, scrh = 255;

#ifdef __APPLE2ENH__
#pragma code-name(push, "LOWCODE")
#else
#pragma code-name(push, "CODE")
#endif

static char url[512];
char video_size;
char enable_subtitles;

int main(void) {
  FILE *url_fp;
  char *last_sep;

  screensize(&scrw, &scrh);

#ifdef __APPLE2ENH__
  videomode(VIDEOMODE_80COL);
#endif

  url_fp = fopen(URL_PASSER_FILE, "r");
  surl_connect_proxy();

  surl_user_agent = "Wozamp for Apple II / "VERSION;

  if (url_fp == NULL) {
    goto out;
  }
  fgets(url, 511, url_fp);
  if ((last_sep = strchr(url, '\n')) != NULL)
    *last_sep = '\0';
  enable_subtitles = fgetc(url_fp);
  video_size = fgetc(url_fp);
  translit_charset = malloc(32);
  fgets(translit_charset, 31, url_fp);
  fclose(url_fp);

  /* Remove filename from URL in advance, so we don't get stuck in
   * a loop if the player crashes for some reason */
  if ((last_sep = strrchr(url, '/')) != NULL) {
    *last_sep = '\0';

    url_fp = fopen(URL_PASSER_FILE, "w");

    if (url_fp) {
      fputs(url, url_fp);
      fclose(url_fp);
    }
    *last_sep = '/';
  }
  reopen_start_device();

  stream_url(url, NULL);

out:
  clrscr();
  init_text();
  exec("WOZAMP", NULL);
}
#pragma code-name(pop)
