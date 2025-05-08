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

char *translit_charset;
char monochrome = 1;
unsigned char scrw = 255, scrh = 255;

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

  if (IS_NULL(url_fp)) {
    goto out;
  }
  fgets(url, 511, url_fp);
  if (IS_NOT_NULL(last_sep = strchr(url, '\n')))
    *last_sep = '\0';
  enable_subtitles = fgetc(url_fp);
  video_size = fgetc(url_fp);
  translit_charset = malloc(32);
  fgets(translit_charset, 31, url_fp);
  fclose(url_fp);

  /* Now unlink this and setup the HGR mono buffers */
  unlink(URL_PASSER_FILE);
  load_hgr_mono_file(2);

  /* Remove filename from URL in advance, so we don't get stuck in
   * a loop if the player crashes for some reason, and put the
   * passer file back */
  if (IS_NOT_NULL(last_sep = strrchr(url, '/'))) {
    *last_sep = '\0';

    url_fp = fopen(URL_PASSER_FILE, "w");

    if (IS_NOT_NULL(url_fp)) {
      fputs(url, url_fp);
      fclose(url_fp);
    }
    *last_sep = '/';
  }
  reopen_start_device();

  surl_stream_av(NULL, url);

out:
  clrscr();
  init_text();
  exec("WOZAMP", NULL);
}
