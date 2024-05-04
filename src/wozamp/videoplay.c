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

static void update_progress(void) {
  unsigned char eta = simple_serial_getc();
  hgr_mixon();
  gotoxy(11, 20); /* strlen("Loading...") + 1 */
  if (eta == 255)
    cputs("(More than 30m remaining)");
  else
    cprintf("(About %ds remaining)   ", eta*8);
}

#pragma code-name(push, "LOWCODE")

static char url[512];

int main(void) {
  unsigned char r, subtitles, size;
  FILE *url_fp;

  videomode(VIDEOMODE_80COL);

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
  simple_serial_putc(subtitles);
  simple_serial_putc(size);

  /* Remove filename from URL in advance, so we don't get stuck in
   * a loop if the player crashes for some reason */
  reopen_start_device();
  if (strchr(url, '/')) {
    *(strrchr(url, '/')) = '\0';
  }
  url_fp = fopen(URL_PASSER_FILE,"w");

  if (url_fp) {
    fputc(subtitles, url_fp);
    fputc(size, url_fp);
    fputs(translit_charset, url_fp);
    fputc('\n', url_fp);
    fputs(url, url_fp);
    fclose(url_fp);
  }
  reopen_start_device();

  init_hgr(1);
  hgr_mixon();

  clrscr();
  /* clear text page 2 */
  memset((char*)0x800, ' '|0x80, 0x400);

  gotoxy(0, 20);
  cputs("Loading...\r\n"
        "Controls: Space:      Play/Pause,             Esc: Quit player\r\n"
        "          Left/Right: Rewind/Forward,         Tab: Toggle subtitles\r\n"
        "          -/=/+:      Volume up/default/down");

read_metadata_again:
  if (kbhit()) {
    if (cgetc() == CH_ESC) {
      init_text();
      simple_serial_putc(SURL_METHOD_ABORT);
      goto out;
    }
  }

  r = simple_serial_getc();
  if (r == SURL_ANSWER_STREAM_METADATA) {
    char *metadata;
    size_t len;
    surl_read_with_barrier((char *)&len, 2);
    len = ntohs(len);
    metadata = malloc(len + 1);
    surl_read_with_barrier(metadata, len);
    metadata[len] = '\0';
    //show_metadata(metadata);
    free(metadata);
    simple_serial_putc(SURL_CLIENT_READY);
    goto read_metadata_again;

  } else if (r == SURL_ANSWER_STREAM_ART) {
    surl_read_with_barrier((char *)HGR_PAGE, HGR_LEN);
    init_hgr(1);
    hgr_mixoff();
    simple_serial_putc(SURL_CLIENT_READY);
    goto read_metadata_again;

  } else if (r == SURL_ANSWER_STREAM_LOAD) {
    update_progress();
    if (kbhit() && cgetc() == CH_ESC)
      simple_serial_putc(SURL_METHOD_ABORT);
    else
      simple_serial_putc(SURL_CLIENT_READY);
    goto read_metadata_again;

  } else if (r == SURL_ANSWER_STREAM_START) {
    videomode(VIDEOMODE_40COL);
    if (!subtitles) {
      hgr_mixoff();
    }
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
