#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <conio.h>
#include "clrzone.h"

int main(int argc, char *argv[]) {
  char x, y;
  
  videomode(VIDEOMODE_80COL);
  for (x = 0; x < 80; x++) {
    for (y = 0; y < 24; y++) {
      cputcxy(x, y, '*');
    }
  }
  cgetc();
  gotoxy(0, 0);
  
  clrzone(2, 2, 78, 2);
  cgetc();

  return 0;
}
