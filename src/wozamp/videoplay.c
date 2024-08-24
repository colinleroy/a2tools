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

char *translit_charset;
char monochrome = 1;

#ifdef __APPLE2ENH__
#pragma code-name(push, "LOWCODE")
#else
#pragma code-name(push, "CODE")
#endif

static void update_progress(void) {
  unsigned char eta = simple_serial_getc();
  hgr_mixon();
  gotoxy(11, 20); /* strlen("Loading...") + 1 */
  if (eta == 255)
    cputs("(More than 30m remaining)");
  else
    cprintf("(About %ds remaining)   ", eta*8);
}

static char url[512];

int main(void) {
  unsigned char r, subtitles, size;
  FILE *url_fp;

#ifdef __APPLE2ENH__
  videomode(VIDEOMODE_80COL);
#endif

  url_fp = fopen(URL_PASSER_FILE, "r");
  surl_connect_proxy();

  if (url_fp == NULL) {
    goto out;
  }
  subtitles = fgetc(url_fp);
  size = fgetc(url_fp);
  translit_charset = malloc(32);
  fgets(translit_charset, 31, url_fp);
  if (strchr(translit_charset, '\n'))
    *strchr(translit_charset, '\n') = '\0';
  fgets(url, 511, url_fp);
  fclose(url_fp);

  surl_start_request(SURL_METHOD_STREAM_AV, url, NULL, 0);
  simple_serial_write(translit_charset, strlen(translit_charset));
  simple_serial_putc('\n');
  simple_serial_putc(monochrome);
  simple_serial_putc(subtitles ? SUBTITLES_AUTO : SUBTITLES_NO);
  simple_serial_putc(size);

  /* Remove filename from URL in advance, so we don't get stuck in
   * a loop if the player crashes for some reason */
  reopen_start_device();
  if (strchr(url, '/')) {
    *(strrchr(url, '/')) = '\0';
  }

  init_hgr(1);
  hgr_mixon();

  url_fp = fopen(URL_PASSER_FILE, "w");

  if (url_fp) {
    fputc(subtitles, url_fp);
    fputc(size, url_fp);
    fputs(translit_charset, url_fp);
    fputc('\n', url_fp);
    fputs(url, url_fp);
    fclose(url_fp);
  }
  reopen_start_device();

  clrscr();
  /* clear text page 2 */
  memset((char*)0x800, ' '|0x80, 0x400);

  gotoxy(0, 20);
#ifdef __APPLE2ENH__
  cputs("Loading...\r\n"
        "Controls: Space:      Play/Pause,             Esc: Quit player\r\n"
        "          Left/Right: Rewind/Forward,         ");
  if (subtitles) {
    cputs(                                             "Tab: Toggle subtitles");
  }
  cputs("\r\n"
        "          -/=/+:      Volume up/default/down  S:   Toggle speed/quality");
#else
  cputs("Loading...\r\n");
#endif

wait_load:
  if (kbhit()) {
    if (cgetc() == CH_ESC) {
      init_text();
      simple_serial_putc(SURL_METHOD_ABORT);
      goto out;
    }
  }

  r = simple_serial_getc();
  if (r == SURL_ANSWER_STREAM_LOAD) {
    update_progress();
    if (kbhit() && cgetc() == CH_ESC)
      simple_serial_putc(SURL_METHOD_ABORT);
    else
      simple_serial_putc(SURL_CLIENT_READY);
    goto wait_load;

  } else if (r == SURL_ANSWER_STREAM_START) {
#ifdef __APPLE2ENH__
    videomode(VIDEOMODE_40COL);
#endif
    hgr_mixoff();
    clrscr();
    surl_stream_av();
    init_text();
  } else {
    init_text();
    clrscr();
    gotoxy(13, 10);
    cputs("Playback error");
    sleep(1);
  }
out:
  exec("WOZAMP", NULL);
}
#pragma code-name(pop)
