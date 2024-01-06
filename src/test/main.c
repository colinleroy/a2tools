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
#include "scroll.h"
#include "scrollwindow.h"

int main(int argc, char *argv[]) {
  int i = 0;
  unsigned *BASL = (unsigned *)0x0028;
  static unsigned fbasl;
  for (i=0; i < 24;i++) {
    gotoxy(0,i);
    printf("%d: %04X", i, *BASL);
    __asm__("lda "CV);
    __asm__("jsr FVTABZ");
    __asm__("lda $28");
    __asm__("sta %v", fbasl);
    __asm__("lda $29");
    __asm__("sta %v+1", fbasl);
    gotoxy(10,i);
    printf(" - %04X", fbasl);
  }
  cgetc();
}
