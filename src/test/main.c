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
  videomode(VIDEOMODE_80COL);
  printf("test1\n");
  printf("test2\n");
  cgetc();
  putchar(0x17);
  cgetc();
  putchar(0x16);
  cgetc();
}
