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
#include "a2_features.h"

char *translit_charset;
char monochrome = 1;
unsigned char scrw = 255, scrh = 255;
char *url_passer_file;

static char url[512];
char video_size;
char enable_subtitles;

int main(void) {
  FILE *url_fp;
  char *last_sep;

  screensize(&scrw, &scrh);

  try_videomode(VIDEOMODE_80COL);

  if (has_80cols) {
    /* Assume it's a 64kB card... */
    url_passer_file = RAM_URL_PASSER_FILE;
  } else {
    url_passer_file = URL_PASSER_FILE;
  }

  url_fp = fopen(url_passer_file, "r");
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
  unlink(url_passer_file);
  load_hgr_mono_file(2);

  /* Remove filename from URL in advance, so we don't get stuck in
   * a loop if the player crashes for some reason, and put the
   * passer file back */
  if (IS_NOT_NULL(last_sep = strrchr(url, '/'))) {
    *last_sep = '\0';

    url_fp = fopen(url_passer_file, "w");

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
