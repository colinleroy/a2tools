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
  simple_serial_open();
  simple_serial_setup_no_irq_regs();

  init_hgr(1);
again:
  hgr_mixon();
  clrscr();
  gotoxy(0, 22);
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
