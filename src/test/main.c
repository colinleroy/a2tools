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

int main(int argc, char *argv[]) {
  memset(0x2000, 0, 0x2000);
  init_hgr(1);
  hgr_mixon();
  clrscr();
  gotoxy(0, 22);
  cputs("Waiting for stream...\n");
  simple_serial_open();
  surl_stream();
  clrscr();
  gotoxy(0, 22);
  cputs("Stream finished!\n");
  hgr_mixon();
  cgetc();
}
