#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <dirent.h>

#include "simple_serial.h"
#include "extended_conio.h"
#include "hgr-convert.h"
#include "stream.h"
#include "ffmpeg.h"

int main(int argc, char *argv[]) {

  if (simple_serial_open() != 0) {
    printf("Can't open serial\n");
    exit(1);
  }
  if (argc < 2) {
    printf("Usage: %s [video]\n", argv[0]);
    exit(1);
  }
  surl_stream_url(argv[1]);
}
