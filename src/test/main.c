#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <conio.h>
#include <unistd.h>
#include "get_filedetails.h"
#include "surl.h"

int main(int argc, char *argv[]) {
  unsigned char t;
  unsigned st;
  unsigned long size;
  char *filename;
  
  surl_connect_proxy();
  get_filedetails("PRODOS", &filename, &size, &t, &st);
  printf("done. filename %s type %d size %zuB st %d\n", filename, size, t, st);
  cgetc();
}
