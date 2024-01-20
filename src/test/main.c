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
  dputc(0x07);
  platform_msleep(200);
  dputc(0x07);
  cgetc();
}
