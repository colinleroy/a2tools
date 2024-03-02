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

static char buf[1024];
int main(int argc, char *argv[]) {
  surl_connect_proxy();
//  surl_start_request(SURL_METHOD_STREAM, "https://static.piaille.fr/media_attachments/files/112/016/730/859/068/160/original/c71d5f0431c7c5f4.mp4", NULL, 0);
again:
  init_hgr(1);
  hgr_mixon();
  clrscr();
  gotoxy(0, 22);
  cputs("press a key to start");
  cgetc();
  clrscr();
  gotoxy(0, 22);
  surl_start_request(SURL_METHOD_STREAM, "/home/colin/Downloads/ba.webm", NULL, 0);
  cputs("Waiting for stream...");

  if (surl_wait_for_stream() != 0) {
    goto again;
  }

  surl_stream();

  clrscr();
  gotoxy(0, 22);
  cputs("Stream finished!\n");
goto again;
  hgr_mixon();
  cgetc();
}
