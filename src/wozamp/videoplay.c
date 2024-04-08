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
#include "pwm_av.h"

char *translit_charset = "ISO646-FR1";
char monochrome = 1;
static char url[512];

int main(void) {
  unsigned char r;
  FILE *url_fp = fopen("/RAM/VIDURL","r");
  surl_connect_proxy();

  if (url_fp == NULL) {
    goto out;
  }
  fgets(url, 511, url_fp);
  fclose(url_fp);

  /* Clear HGR buffers */
  memset((char *)HGR_PAGE, 0, HGR_LEN);
  memset((char *)HGR_PAGE2, 0, HGR_LEN);
  surl_start_request(SURL_METHOD_STREAM_AV, url, NULL, 0);
  simple_serial_write(translit_charset, strlen(translit_charset));
  simple_serial_putc('\n');
  simple_serial_putc(monochrome);
  simple_serial_putc(HGR_SCALE_MIXHGR);

  init_hgr(1);
  hgr_mixoff();

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
    hgr_mixoff();
    simple_serial_putc(SURL_CLIENT_READY);
    goto read_metadata_again;

  } else if (r == SURL_ANSWER_STREAM_START) {
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
out:
  reopen_start_device();
  if (strchr(url, '/')) {
    *(strrchr(url, '/')) = '\0';
  }
  url_fp = fopen("/RAM/VIDURL","w");

  if (url_fp) {
    fputs(url, url_fp);
    fclose(url_fp);
  }
  exec("WOZAMP", NULL);
}
