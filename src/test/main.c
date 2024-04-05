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
#include "surl.h"
#include "pwm_av.h"

char *translit_charset = "ISO646-FR1";
char monochrome = 1;

int main(int argc, char *argv[]) {
  unsigned char r;
  surl_connect_proxy();

  /* Clear HGR buffers */
  memset((char *)HGR_PAGE, 0, HGR_LEN);
  memset((char *)HGR_PAGE2, 0, HGR_LEN);
  surl_start_request(SURL_METHOD_STREAM_AV, "file:///home/colin/Downloads/ba.webm", NULL, 0);
  simple_serial_write(translit_charset, strlen(translit_charset));
  simple_serial_putc('\n');
  simple_serial_putc(monochrome);
  simple_serial_putc(HGR_SCALE_MIXHGR);

  init_hgr(1);
  hgr_mixon();

read_metadata_again:
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
    hgr_mixon();
    simple_serial_putc(SURL_CLIENT_READY);
    goto read_metadata_again;

  } else if (r == SURL_ANSWER_STREAM_START) {
    simple_serial_putc(SURL_CLIENT_READY);
    pwm();
    init_text();
    // clrzone(0, 20, scrw - 1, 23);
    // stp_print_footer();
  } else {
    init_text();
    // gotoxy(center_x, 10);
    cputs("Playback error");
    sleep(1);
  }
}
