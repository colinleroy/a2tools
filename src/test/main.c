#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <conio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <accelerator.h>
#include <joystick.h>
#include "clrzone.h"
#include "dputc.h"
#include "scroll.h"
#include "scrollwindow.h"
#include "platform.h"


int main(int argc, char *argv[]) {
  int i,j;
  clrscr();
  videomode(VIDEOMODE_80COL);
  for (i = 0; i < 80; i++) {
    for (j = 0; j < 24; j++) {
      cputc('*');
    }
  }
  clrzone(2, 2, 10, 10);

  clrzone(0, 12, 78, 12);
  clrzone(0, 14, 79, 12);
  cgetc();
  set_scrollwindow(1,23);
  set_hscrollwindow(1,79);
  clrscr();
  cgetc();

  for (i = 0; i < 80; i++) {
    for (j = 0; j < 24; j++) {
      cputc('*');
    }
  }
  gotoxy(5, 5);
  cputs("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  cgetc();
  clrzone(7, 5, 17, 5);
  cgetc();
}
