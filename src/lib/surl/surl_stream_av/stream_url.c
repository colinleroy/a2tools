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
#include "scrollwindow.h"

#pragma code-name (push, STREAM_URL_SEGMENT)

extern char enable_subtitles;
extern char video_size;
extern char *translit_charset;
extern unsigned char scrh;

static void update_progress(void) {
  unsigned char eta = simple_serial_getc();
  hgr_mixon();
  gotoxy(11, 0); /* strlen("Loading...") + 1 */
  if (eta == 255)
    cputs("(Opening stream...)");
  else if (eta == 254)
    cputs("(More than 30m remaining)");
  else
    cprintf("(About %ds remaining)   ", eta*8);
}

int stream_url(char *url, char *subtitles_url) {
  char r, got_art = 0;

  surl_start_request(NULL, 0, url, SURL_METHOD_STREAM_AV);

  simple_serial_puts_nl(translit_charset);
  simple_serial_putc(1); /* Monochrome */
  if (enable_subtitles) {
    simple_serial_putc(subtitles_url != NULL ? SUBTITLES_URL : SUBTITLES_AUTO); /* Enable subtitles */
    if (subtitles_url != NULL) {
      simple_serial_puts(subtitles_url);
      simple_serial_putc('\n');
    }
  } else {
    simple_serial_putc(SUBTITLES_NO);
  }

  /* Do all of the slow things before finishing to send STREAM_AV parameters,
   * otherwise proxy's going to start sending _STREAM_LOAD ETAs before we can
   * answer.
   */
  clrscr();
  set_scrollwindow(20, scrh);
  init_hgr(1);
  hgr_mixon();
  gotoxy(0, 0);

  /* clear text page 2 */
  memset((char*)0x800, ' '|0x80, 0x400);

#ifdef __APPLE2ENH__
  cputs("Loading...\r\n"
        "Controls: Space:      Play/Pause,             Esc:     Quit player,\r\n"
        "          Left/Right: Rewind/Forward 10s,     Up/Down: Rewind/Forward 1m\r\n"
        "          -/=/+:      Volume up/default/down  S:       Toggle speed/quality");
#else
  cputs("Loading...\r\n"
        "Space: play/pause, Esc: Quit player,\r\n"
        "Left/right: Rew/Fwd 10s, U/J: 60s\r\n"
        "-/=/+: Vol up/def/down, S: spd/quality ");
#endif

  /* Ready, send last parameter */
  simple_serial_putc(video_size);

wait_load:
  r = simple_serial_getc();
  if (r == SURL_ANSWER_STREAM_LOAD) {
    update_progress();
    if (kbhit() && cgetc() == CH_ESC)
      simple_serial_putc(SURL_METHOD_ABORT);
    else
      simple_serial_putc(SURL_CLIENT_READY);
    goto wait_load;
  } else if (r == SURL_ANSWER_STREAM_ART) {
    surl_read_with_barrier((char *)HGR_PAGE, HGR_LEN);
    got_art = 1;
    simple_serial_putc(SURL_CLIENT_READY);
    goto wait_load;
  } else if (r == SURL_ANSWER_STREAM_START) {
    if (simple_serial_getc() == SURL_VIDEO_PORT_NOK) {
      clrscr();
      cputs("Warning: proxy couldn't open video aux_tty.\r\nNo video available.");
      sleep(5);
    }
    if (!got_art) {
      bzero((char *)HGR_PAGE, HGR_LEN);
    }
#ifdef __APPLE2ENH__
    videomode(VIDEOMODE_40COL);
#endif
    hgr_mixoff();
    set_scrollwindow(0, scrh);
    clrscr();
    surl_stream_av();
#ifdef __APPLE2ENH__
    videomode(VIDEOMODE_80COL);
#endif
  } else {
    clrscr();
    cputs("Playback error");
    sleep(1);
  }
  set_scrollwindow(0, scrh);
  return 0;
}
#pragma code-name (pop)
