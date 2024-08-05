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

extern uint8 splash_hgr;

#pragma code-name(push, "LOWCODE")

static void update_progress(void) {
  unsigned char eta = simple_serial_getc();
  hgr_mixon();
  gotoxy(11, 20); /* strlen("Loading...") + 1 */
  if (eta == 255)
    cputs("(More than 30m remaining)");
  else
    cprintf("(About %ds remaining)   ", eta*8);
}

int stream_url(char *url) {
  char r;

  surl_start_request(SURL_METHOD_STREAM_AV, url, NULL, 0);
  simple_serial_write(translit_charset, strlen(translit_charset));
  simple_serial_putc('\n');
  simple_serial_putc(1); /* Monochrome */
  simple_serial_putc(0); /* Enable subtitles */
  simple_serial_putc(video_size);

  clrscr();
  /* clear text page 2 */
  memset((char*)0x800, ' '|0x80, 0x400);

  cputs("Loading...\r\n"
        "Controls: Space:      Play/Pause,             Esc: Quit player,\r\n"
        "          Left/Right: Rewind/Forward,         ");
  cputs("\r\n"
        "          -/=/+:      Volume up/default/down  S:   Toggle speed/quality");

read_metadata_again:
  if (kbhit()) {
    if (cgetc() == CH_ESC) {
      init_text();
      simple_serial_putc(SURL_METHOD_ABORT);
      return -1;
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
    hgr_mixoff();
    clrscr();
    surl_stream_av();
  } else {
    clrscr();
    cputs("Playback error");
    sleep(1);
  }
  return 0;
}
#pragma code-name(pop)
